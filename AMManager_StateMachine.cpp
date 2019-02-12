/*
 * AMManager.cpp
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 */

#include "AMManager.h"


static const char* _MODULE_ = "[AMM]...........";
#define _EXPR_	(_defdbg && !IS_ISR())


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
        		// inicia con la calibración de medida por defecto
        		//_driver->initEnergyIC(NULL, 0, NULL, 0);
        		_driver->initEnergyIC((uint16_t*)_meter_cal_values, Blob::AMCalibRegCount, (uint16_t*)_meas_cal_values, Blob::AMCalibRegCount);
        		if(!_driver->ready()){
        			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_DRV. Reintentando en 1sec");
        		}
        	}
            DEBUG_TRACE_I(_EXPR_, _MODULE_, "Driver OK!");

        	// inicializa el estado de las medidas realizando una medida inicial pero sin notificarla
        	_measure(false);

        	// obtiene los parámetros de calibración
        	if(_driver->getMeterCalib(_amdata.cfg.calibData.meterRegs, Blob::AMCalibRegCount) != 0){
        		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Error leyendo MeterCalib");
        	}
        	if(_driver->getMeasureCalib(_amdata.cfg.calibData.measRegs, Blob::AMCalibRegCount) != 0){
        		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Error leyendo MeasureCalib");
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
         	Blob::SetRequest_t<Blob::AMCfgData_t>* req = (Blob::SetRequest_t<Blob::AMCfgData_t>*)st_msg->msg;
        	MBED_ASSERT(req);

        	// si no hay errores, actualiza la configuración
        	if(req->_error.code == Blob::ErrOK){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Iniciando actualización");
				_updateConfig(req->data, req->keys, req->_error);
        	}
        	// si hay errores en el mensaje o en la actualización, devuelve resultado sin hacer nada
        	if(req->_error.code != Blob::ErrOK){
        		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Notificando error %s", req->_error.descr);
        		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<Blob::AMCfgData_t>* resp = new Blob::Response_t<Blob::AMCfgData_t>(req->idTrans, req->_error, _amdata.cfg);
				MBED_ASSERT(resp);

				if(_json_supported){
					cJSON* jresp = JsonParser::getJsonFromResponse(*resp);
					if(jresp){
						char* jmsg = cJSON_Print(jresp);
						cJSON_Delete(jresp);
						MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
						Heap::memFree(jmsg);
						delete(resp);
						Heap::memFree(pub_topic);
						return State::HANDLED;
					}
				}

				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::AMCfgData_t>), &_publicationCb);
				delete(resp);
				Heap::memFree(pub_topic);
				return State::HANDLED;
        	}

        	// si todo ha ido bien, almacena en el sistema de ficheros
        	saveConfig();

        	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Config actualizada");

        	// al cambiar la configuración, cambia la cadencia de envío de medidas
        	stopMeasureWork();
        	startMeasureWork();

        	// si está habilitada la notificación de actualización, lo notifica
        	if((_amdata.cfg.updFlagMask & Blob::EnableAMCfgUpdNotif) != 0){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Notificando actualización");
				char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<Blob::AMCfgData_t>* resp = new Blob::Response_t<Blob::AMCfgData_t>(req->idTrans, req->_error, _amdata.cfg);
				MBED_ASSERT(resp);
				if(_json_supported){
					cJSON* jresp = JsonParser::getJsonFromResponse(*resp);
					if(jresp){
						char* jmsg = cJSON_Print(jresp);
						cJSON_Delete(jresp);
						MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
						Heap::memFree(jmsg);
						delete(resp);
						Heap::memFree(pub_topic);
						return State::HANDLED;
					}
					else{
						DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error on getJsonFromResponse <%s>", resp->error.descr);
						delete(resp);
					}
				}
				else{
					MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::AMCfgData_t>), &_publicationCb);
					delete(resp);
				}
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
			Blob::Response_t<Blob::AMCfgData_t>* resp = new Blob::Response_t<Blob::AMCfgData_t>(req->idTrans, req->_error, _amdata.cfg);
			MBED_ASSERT(resp);
			if(_json_supported){
				cJSON* jresp = JsonParser::getJsonFromResponse(*resp);
				if(jresp){
					char* jmsg = cJSON_Print(jresp);
					cJSON_Delete(jresp);
					MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
					Heap::memFree(jmsg);
					delete(resp);
					Heap::memFree(pub_topic);
					return State::HANDLED;
				}
			}

			MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::AMCfgData_t>), &_publicationCb);
			delete(resp);

        	// libera la memoria asignada al topic de publicación
			Heap::memFree(pub_topic);
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
			Blob::Response_t<Blob::AMStatData_t>* resp = new Blob::Response_t<Blob::AMStatData_t>(req->idTrans, req->_error, _amdata.stat);
			MBED_ASSERT(resp);
			if(_json_supported){
				cJSON* jresp = JsonParser::getJsonFromResponse(*resp);
				if(jresp){
					char* jmsg = cJSON_Print(jresp);
					cJSON_Delete(jresp);
					MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
					Heap::memFree(jmsg);
					delete(resp);
					Heap::memFree(pub_topic);
					return State::HANDLED;
				}
			}

			MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::AMStatData_t>), &_publicationCb);
			delete(resp);

        	// libera la memoria asignada al topic de publicación
			Heap::memFree(pub_topic);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en get/boot
        case RecvBootGet:{
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/boot/%s", _pub_topic_base);

			if(_json_supported){
				cJSON* jboot = JsonParser::getJsonFromObj<Blob::AMBootData_t>(_amdata);
				if(jboot){
					char* jmsg = cJSON_Print(jboot);
					cJSON_Delete(jboot);
					MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
					Heap::memFree(jmsg);
					Heap::memFree(pub_topic);
					return State::HANDLED;
				}
			}

			MQ::MQClient::publish(pub_topic, &_amdata, sizeof(Blob::AMBootData_t), &_publicationCb);
			Heap::memFree(pub_topic);

			// una vez notificado el boot inicia el proceso de medida
			startMeasureWork();

            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en set/load/$
        case RecvLoadSet:{
        	// actualiza el control de carga actual
        	Blob::AMLoadData_t ld = *(Blob::AMLoadData_t*)st_msg->msg;
        	if(ld.outValue != _load_data.outValue){
        		_load_data = ld;
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Carga actualizada al %d",_load_data.outValue);
        		// planifica una nueva notificación de medida en 3 segundos
        		_instant_meas_counter = SecondsToForcedNotifOnLoadChange;
        	}
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


