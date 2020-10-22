/*
 * AMManager.cpp
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 */

#include "AMManager.h"


static const char* _MODULE_ = "[AMM]...........";
#define _EXPR_	(!IS_ISR())


//------------------------------------------------------------------------------------
State::StateResult AMManager::Init_EventHandler(State::StateEvent* se){
	State::Msg* st_msg = (State::Msg*)se->oe->value.p;
    switch((int)se->evt){
        case State::EV_ENTRY:{
        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando recuperación de datos...");

        	// recupera los datos de memoria NV
        	restoreConfig();

        	// desactiva la notificación forzada
        	_forced_notification = false;
        	_forced_measure = false;

        	// realiza la suscripción local ej: "[get|set]/[cfg|value]/energy"
        	char* sub_topic_local = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
        	MBED_ASSERT(sub_topic_local);
        	sprintf(sub_topic_local, "set/+/%s", _sub_topic_base);
        	if(MQ::MQClient::subscribe(sub_topic_local, new MQ::SubscribeCallback(this, &AMManager::subscriptionCb)) == MQ::SUCCESS){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Sucripción LOCAL hecha a %s", sub_topic_local);
        	}
        	else{
        		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_SUBSC en la suscripción LOCAL a %s", sub_topic_local);
        	}
        	sprintf(sub_topic_local, "get/+/%s", _sub_topic_base);
        	if(MQ::MQClient::subscribe(sub_topic_local, new MQ::SubscribeCallback(this, &AMManager::subscriptionCb)) == MQ::SUCCESS){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Sucripción LOCAL hecha a %s", sub_topic_local);
        	}
        	else{
        		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_SUBSC en la suscripción LOCAL a %s", sub_topic_local);
        	}
        	Heap::memFree(sub_topic_local);

        	// inicializa el driver con los datos de calibración
            DEBUG_TRACE_D(_EXPR_, _MODULE_, "Iniciando drivers!");
            for(auto drv = _driver_list.begin(); drv != _driver_list.end(); ++drv){
        		DriverObj* dobj = (*drv);
        		AMDriver* am_driver = dobj->drv;
        		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando Driver <%s>...", am_driver->getVersion());
        		am_driver->initEnergyIC();
				bool result = am_driver->ready();
				if(result){
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "Driver <%s> OK!", am_driver->getVersion());
				}
				else{
					DEBUG_TRACE_W(_EXPR_, _MODULE_, "Driver <%s> ERR_INIT", am_driver->getVersion());
				}
            }

        	// marca como componente iniciado
        	_ready = true;
            return State::HANDLED;
        }

        case State::EV_TIMED:{
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/cfg/set
        case RecvCfgSet:{
         	Blob::SetRequest_t<metering_manager>* req = (Blob::SetRequest_t<metering_manager>*)st_msg->msg;
        	MBED_ASSERT(req);

        	// si no hay errores, actualiza la configuración
        	if(req->_error.code == Blob::ErrOK){
        		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando actualización");
				_updateConfig(req->data, req->_error);
        	}
        	// si hay errores en el mensaje o en la actualización, devuelve resultado sin hacer nada
        	if(req->_error.code != Blob::ErrOK){
        		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Notificando error %s", req->_error.descr);
        		_responseWithConfig(req->idTrans, req->_error);
				return State::HANDLED;
        	}

        	// si todo ha ido bien, almacena en el sistema de ficheros
        	saveConfig();

        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Config actualizada");

        	// si está habilitada la notificación de actualización, lo notifica
        	if((_amdata.cfg.updFlags & MeteringManagerCfgUpdNotif) != 0){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Notificando actualización");
        		_responseWithConfig(req->idTrans, req->_error);
        	}
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/cfg/get
        case RecvCfgGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;
        	_responseWithConfig(req->idTrans, req->_error);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/value/get
        case RecvStatGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;
        	_responseWithState(req->idTrans, req->_error);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en get/boot
        case RecvBootGet:{
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/boot/%s", _pub_topic_base);
			Blob::NotificationData_t<metering_manager> *notif = new Blob::NotificationData_t<metering_manager>(_amdata);
			MBED_ASSERT(notif);
			if(_json_supported){
				cJSON* jboot = JsonParser::getJsonFromNotification(*notif);
				MBED_ASSERT(jboot);
				MQ::MQClient::publish(pub_topic, &jboot, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jboot);
			}
			else {
				MQ::MQClient::publish(pub_topic, notif, sizeof(Blob::NotificationData_t<metering_manager>), &_publicationCb);
			}
			delete(notif);
			Heap::memFree(pub_topic);
			startMeasureWork();
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en set/load/$
        case RecvLoadSet:{
        	// actualiza el control de carga actual
        	Blob::AMLoadData_t ld = *(Blob::AMLoadData_t*)st_msg->msg;
        	for(int i=0; i<MeteringManagerCfgMaxNumAnalyzers; i++){
        		if(ld.ids[i] >= 0){
        			if(_amdata.stat.loadPercent[ld.ids[i]] != ld.loads[i]){
        				_amdata.stat.loadPercent[ld.ids[i]] = ld.loads[i];
        				DEBUG_TRACE_I(_EXPR_, _MODULE_, "Carga del Analiz=%d actualizada al %d", ld.ids[i], ld.loads[i]);
        				// planifica una nueva notificación de medida en 3 segundos
        				_instant_meas_counter = SecondsToForcedNotifOnLoadChange;
        			}
        		}
        	}
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en get/boot
        case RecvStopSet:{
        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Deteniendo medidas electricas");
			stopMeasureWork();
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en get/boot
        case RecvRestartSet:{
        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Reanudando medidas electricas");
			startMeasureWork();
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en set/load/$
        case RecvForcedMeasure:{
        	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Realiza medida solicitada y notifica");
        	_measure(false);
        	_notifyState();
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en set/load/$
        case TimedMeasureEvt:{
        	_measure(true);
            return State::HANDLED;
        }

        case State::EV_EXIT:{
            nextState();
            return State::HANDLED;
        }

        default:{
        	return State::IGNORED;
        }

     }
}


