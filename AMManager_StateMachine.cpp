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
            DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando driver!");
        	while(!_driver->ready()){
        		// inicia por defecto si es un driver EMi10 YTL
        		if(strcmp(_driver->getVersion(), (const char*)VERS_METERING_NAME(VERS_METERING_EMi10_YTL)) == 0){
        			_driver->initEnergyIC();
        		}
        		// ajusta la calibración si es un driver M90E26
        		else if(strcmp(_driver->getVersion(), VERS_METERING_NAME(VERS_METERING_M90E26)) == 0){
        			_driver->initEnergyIC((uint16_t*)_meter_cal_values, MeteringAnalyzerCfgCalibRegCount, (uint16_t*)_meas_cal_values, MeteringAnalyzerCfgCalibRegCount);
        		}
        		if(!_driver->ready()){
        			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_DRV. Reintentando en 1sec");
        			Thread::wait(1000);
        		}
        	}
            DEBUG_TRACE_I(_EXPR_, _MODULE_, "Driver OK!");

        	// inicializa el estado de las medidas realizando una medida inicial pero sin notificarla
        	_measure(false);

			// obtiene los parámetros de calibración de cada analizador en caso del M90E26
        	if(strcmp(_driver->getVersion(), VERS_METERING_NAME(VERS_METERING_M90E26)) == 0){
        		for(int i=0; i<_driver->getNumAnalyzers(); i++){
					if(_driver->getMeterCalib(_amdata.analyzers[i].cfg.calibData.meterRegs, MeteringAnalyzerCfgCalibRegCount, i) != 0){
						DEBUG_TRACE_W(_EXPR_, _MODULE_, "Error leyendo MeterCalib AN=%d", i);
					}
					if(_driver->getMeasureCalib(_amdata.analyzers[i].cfg.calibData.measRegs, MeteringAnalyzerCfgCalibRegCount, i) != 0){
						DEBUG_TRACE_W(_EXPR_, _MODULE_, "Error leyendo MeasureCalib AN=%d", i);
					}
        		}
        	}

        	// chequea el estado del driver para ver si está en estado de error
        	uint16_t sys_stat;
        	if(_driver->getSysStatus(&sys_stat) != 0){
				DEBUG_TRACE_W(_EXPR_, _MODULE_, "Error leyendo SystemStatus");
			}
        	else{
        		if(sys_stat & (uint16_t)_driver->ErrMeasCalib){
        			DEBUG_TRACE_W(_EXPR_, _MODULE_, "Error SYS_MEAS_CALIB");
        		}
        		if(sys_stat & (uint16_t)_driver->ErrMeterCalib){
        			DEBUG_TRACE_W(_EXPR_, _MODULE_, "Error SYS_METER_CALIB");
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
        		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<metering_manager>* resp = new Blob::Response_t<metering_manager>(req->idTrans, req->_error, _amdata);
				MBED_ASSERT(resp);

				if(_json_supported){
					cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectCfg);
					MBED_ASSERT(jresp);
					MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
					cJSON_Delete(jresp);
				}
				else {
					MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<metering_manager>), &_publicationCb);
				}
				delete(resp);
				Heap::memFree(pub_topic);
				return State::HANDLED;
        	}

        	// si todo ha ido bien, almacena en el sistema de ficheros
        	saveConfig();

        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Config actualizada");

        	// al cambiar la configuración, cambia la cadencia de envío de medidas
        	stopMeasureWork();
        	startMeasureWork();

        	// si está habilitada la notificación de actualización, lo notifica
        	if((_amdata.cfg.updFlags & MeteringManagerCfgUpdNotif) != 0){
        		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Notificando actualización");
				char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<metering_manager>* resp = new Blob::Response_t<metering_manager>(req->idTrans, req->_error, _amdata);
				MBED_ASSERT(resp);
				if(_json_supported){
					cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectCfg);
					MBED_ASSERT(jresp);
					MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
					cJSON_Delete(jresp);
				}
				else{
					MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<metering_manager>), &_publicationCb);
				}
				delete(resp);
				Heap::memFree(pub_topic);
        	}
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/cfg/get
        case RecvCfgGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;
        	// prepara el topic al que responder
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

			// responde con los datos solicitados y con los errores (si hubiera) de la decodificación de la solicitud
			Blob::Response_t<metering_manager>* resp = new Blob::Response_t<metering_manager>(req->idTrans, req->_error, _amdata);
			MBED_ASSERT(resp);
			if(_json_supported){
				cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectCfg);
				MBED_ASSERT(jresp);
				MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jresp);
			}
			else{
				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<metering_manager>), &_publicationCb);
			}
			delete(resp);
			Heap::memFree(pub_topic);
			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Enviada respuesta con configuracion solicitada");
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/value/get
        case RecvStatGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;
        	// prepara el topic al que responder
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/value/%s", _pub_topic_base);

			// responde con los datos solicitados y con los errores (si hubiera) de la decodificación de la solicitud
			Blob::Response_t<metering_manager>* resp = new Blob::Response_t<metering_manager>(req->idTrans, req->_error, _amdata);
			MBED_ASSERT(resp);
			if(_json_supported){
				cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectState);
				MBED_ASSERT(jresp);
				MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jresp);
			}
			else{
				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<metering_manager>), &_publicationCb);
			}
			delete(resp);
			Heap::memFree(pub_topic);
			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Enviada respuesta con estado solicitado");
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

        case State::EV_EXIT:{
            nextState();
            return State::HANDLED;
        }

        default:{
        	return State::IGNORED;
        }

     }
}


