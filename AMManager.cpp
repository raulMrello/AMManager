/*
 * AMManager.cpp
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 */

#include "AMManager.h"
//------------------------------------------------------------------------------------
//-- PRIVATE TYPEDEFS ----------------------------------------------------------------
//------------------------------------------------------------------------------------

static const char* _MODULE_ = "[AMM]...........";
#define _EXPR_	(_defdbg && !IS_ISR())

// Valores de referencia de Manuel Ariza
//------------------------------------------------------------------------------------
static const uint16_t meter_cal_values[]  = {
	0x5678, //CalStart, 0x5678
	//0, 	//LSB
	22, 	//PLconstH,
	51326, 	//PLconstL,
	4488, 	//Lgain,
	41, 	//Lphi,
	0, 		//Ngain,
	0, 		//Nphi,
	3221, 	//PStartTh,
	0, 		//PNolTh,
	3221, 	//QStartTh,
	0, 		//QNolTh,
	27682, 	//MMode,
	24302, 	//CSOne
};

//------------------------------------------------------------------------------------
static const uint16_t meas_cal_values[]  = {
	0x5678, //AdjStart,
	28595, //Ugain,
	25151, //IgainL,
	30000, //IgainN,
	0x0000, //Uoffset,
	0x0000, //IoffsetL,
	0x0000, //IoffsetN,
	65437, //PoffsetL,
	0x0000, //QoffsetL,
	65505, //PoffsetN,
	0x0000, //QoffsetN,
	47332, //CSTwo,0xD294
};
 
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
AMManager::AMManager(AMDriver* driver, FSManager* fs, bool defdbg) : ActiveModule("AMM", osPriorityNormal, 3096, fs, defdbg) {

	// Establece el soporte de JSON
	_json_supported = false;
	#if AMMANAGER_ENABLE_JSON_SUPPORT == 1
	_json_supported = true;
	#endif

	// referencia el driver a través de la interfaz que expone
	_driver = driver;

	// Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &AMManager::publicationCb);

    // crea el timer para el worker de medida
	_meas_tmr = new RtosTimer(callback(this, &AMManager::eventMeasureWorkCb), osTimerPeriodic, "AMMeasWork");

}


//------------------------------------------------------------------------------------
void AMManager::startMeasureWork() {
	_instant_meas_counter = _amdata.cfg.measPeriod / (DefaultMeasurePeriod/1000);
	// realiza una medida con una cadencia dada
	_meas_tmr->start(DefaultMeasurePeriod);
}


//------------------------------------------------------------------------------------
void AMManager::stopMeasureWork() {
	_meas_tmr->stop();
}


//------------------------------------------------------------------------------------
osStatus AMManager::putMessage(State::Msg *msg){
    osStatus ost = _queue.put(msg, ActiveModule::DefaultPutTimeout);
    if(ost != osOK){
        DEBUG_TRACE_E(_EXPR_, _MODULE_, "QUEUE_PUT_ERROR %d", ost);
    }
    return ost;
}



//------------------------------------------------------------------------------------
//-- PROTECTED METHODS IMPLEMENTATION ------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
void AMManager::subscriptionCb(const char* topic, void* msg, uint16_t msg_len){

    // si es un comando para actualizar los parámetros minmax...
    if(MQ::MQClient::isTokenRoot(topic, "set/cfg")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::SetRequest_t<Blob::AMCfgData_t>* req = NULL;
        bool json_decoded = false;
		if(_json_supported){
			req = (Blob::SetRequest_t<Blob::AMCfgData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::AMCfgData_t>));
			MBED_ASSERT(req);
			if(!(json_decoded = JsonParser::getSetRequestFromJson(*req, (char*)msg))){
				Heap::memFree(req);
				DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_JSON. Decodificando el mensaje");
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::SetRequest_t<Blob::AMCfgData_t>)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        if(!json_decoded){
			// el mensaje es un blob tipo Blob::AMCfgData_t
			req = (Blob::SetRequest_t<Blob::AMCfgData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::AMCfgData_t>));
			MBED_ASSERT(req);
			*req = *((Blob::SetRequest_t<Blob::AMCfgData_t>*)msg);
        }
        op->sig = RecvCfgSet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar la configuración
    if(MQ::MQClient::isTokenRoot(topic, "get/cfg")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::GetRequest_t* req = NULL;
        bool json_decoded = false;
        if(_json_supported){
			req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
			MBED_ASSERT(req);
			if(!(json_decoded = JsonParser::getGetRequestFromJson(*req, (char*)msg))){
				Heap::memFree(req);
			}
        }

        // Antes de nada, chequea que el tamaño de la zona horaria es correcto, en caso contrario, descarta el topic
        if(!json_decoded && msg_len != sizeof(Blob::GetRequest_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
        if(!json_decoded){
        	req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
        	MBED_ASSERT(req);
        	*req = *((Blob::GetRequest_t*)msg);
        }
		op->sig = RecvCfgGet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar los datos de arranque
    if(MQ::MQClient::isTokenRoot(topic, "get/boot")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        op->sig = RecvBootGet;
		// apunta a los datos
		op->msg = NULL;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar el estado
    if(MQ::MQClient::isTokenRoot(topic, "get/value")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::GetRequest_t* req = NULL;
        bool json_decoded = false;
        if(_json_supported){
			req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
			MBED_ASSERT(req);
			if(!(json_decoded = JsonParser::getGetRequestFromJson(*req, (char*)msg))){
				Heap::memFree(req);
			}
        }

        // Antes de nada, chequea que el tamaño de la zona horaria es correcto, en caso contrario, descarta el topic
        if(!json_decoded && msg_len != sizeof(Blob::GetRequest_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
        if(!json_decoded){
        	req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
        	MBED_ASSERT(req);
        	*req = *((Blob::GetRequest_t*)msg);
        }
		op->sig = RecvStatGet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para ajustar el % de activación de la carga
    if(MQ::MQClient::isTokenRoot(topic, "set/load")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::AMLoadData_t* load = NULL;
        bool json_decoded = false;
		if(_json_supported){
			load = (Blob::AMLoadData_t*)Heap::memAlloc(sizeof(Blob::AMLoadData_t));
			MBED_ASSERT(load);
			if(!(json_decoded = JsonParser::getObjFromJson(*load, (char*)msg))){
				Heap::memFree(load);
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::AMLoadData_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::LightCfgData_t
        if(!json_decoded){
			// el mensaje es un blob tipo Blob::AMCfgData_t
        	load = (Blob::AMLoadData_t*)Heap::memAlloc(sizeof(Blob::AMLoadData_t));
			MBED_ASSERT(load);
			*load = *((Blob::AMLoadData_t*)msg);
        }
        op->sig = RecvLoadSet;
		// apunta a los datos
		op->msg = load;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }
    DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_TOPIC. No se puede procesar el topic [%s]", topic);
}

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
        		_driver->initEnergyIC((uint16_t*)meter_cal_values, Blob::AMCalibRegCount, (uint16_t*)meas_cal_values, Blob::AMCalibRegCount);
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


//------------------------------------------------------------------------------------
osEvent AMManager:: getOsEvent(){
	return _queue.get();
}



//------------------------------------------------------------------------------------
void AMManager::publicationCb(const char* topic, int32_t result){

}


//------------------------------------------------------------------------------------
bool AMManager::checkIntegrity(){
	#warning TODO
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "~~~~ TODO ~~~~ AMManager::checkIntegrity");
	// Hacer lo que corresponda
	// ...
	if(_amdata.cfg.measPeriod < Blob::AMMinMeasPeriod)
		return false;

	return true;
}


//------------------------------------------------------------------------------------
void AMManager::setDefaultConfig(){
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "~~~~ TODO ~~~~ AMManager::setDefaultConfig");
	// por defecto sólo notifica las medidas instantáneas cada 15min
	_amdata.cfg.evtFlagMask = Blob::AMInstantMeasureEvt;
	_amdata.cfg.measPeriod = Blob::AMDefaultMeasPeriod;

	// precarga los valores de calibración por defecto
	for(int i=0;i<sizeof(meter_cal_values)/sizeof(meter_cal_values[0]);i++){
		_amdata.cfg.calibData.meterRegs[i] = meter_cal_values[i];
	}
	for(int i=sizeof(meter_cal_values)/sizeof(meter_cal_values[0]);i<Blob::AMCalibRegCount;i++){
		_amdata.cfg.calibData.meterRegs[i] = 0;
	}
	for(int i=0;i<sizeof(meas_cal_values)/sizeof(meas_cal_values[0]);i++){
		_amdata.cfg.calibData.measRegs[i] = meas_cal_values[i];
	}
	for(int i=sizeof(meas_cal_values)/sizeof(meas_cal_values[0]);i<Blob::AMCalibRegCount;i++){
		_amdata.cfg.calibData.measRegs[i] = 0;
	}

	// guarda la configuración
	saveConfig();
}


//------------------------------------------------------------------------------------
void AMManager::restoreConfig(){
	uint32_t crc = 0;
	// establece datos por defecto
	_load_data.outValue = 0;
	_amdata.stat.flags = Blob::AMNoEvents;

	// inicia la recuperación de datos
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recuperando datos de memoria NV...");
	bool success = true;
	if(!restoreParameter("AMUpdFlags", &_amdata.cfg.updFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo UpdFlags!");
		success = false;
	}
	if(!restoreParameter("AMEvtFlags", &_amdata.cfg.evtFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo EvtFlags!");
		success = false;
	}
	if(!restoreParameter("AMMeasPeriod", &_amdata.cfg.measPeriod, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo EvtFlags!");
		success = false;
	}
	if(!restoreParameter("AMMinMaxData", &_amdata.cfg.minmaxData, sizeof(Blob::AMMinMaxData_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo IVData!");
		success = false;
	}
	if(!restoreParameter("AMCalibData", &_amdata.cfg.calibData, sizeof(Blob::AMCalibData_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo AstData!");
		success = false;
	}
	if(!restoreParameter("AMChecksum", &crc, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo Checksum!");
		success = false;
	}

	if(success){
		// chequea el checksum crc32 y después la integridad de los datos
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Datos recuperados. Chequeando integridad...");
		if(Blob::getCRC32(&_amdata.cfg, sizeof(Blob::AMCfgData_t)) != crc){
			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_CFG. Ha fallado el checksum");
		}
    	else if(!checkIntegrity()){
    		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_CFG. Ha fallado el check de integridad.");
    	}
    	else{
    		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Check de integridad OK!");
    		return;
    	}
	}
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_FS. Error en la recuperación de datos. Establece configuración por defecto");
	setDefaultConfig();
}


//------------------------------------------------------------------------------------
void AMManager::saveConfig(){
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Guardando datos en memoria NV...");
	if(!saveParameter("AMUpdFlags", &_amdata.cfg.updFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando UpdFlags!");
	}
	if(!saveParameter("AMEvtFlags", &_amdata.cfg.evtFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando EvtFlags!");
	}
	if(!saveParameter("AMMeasPeriod", &_amdata.cfg.measPeriod, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando EvtFlags!");
	}
	if(!saveParameter("AMMinMaxData", &_amdata.cfg.minmaxData, sizeof(Blob::AMMinMaxData_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando IVData!");
	}
	if(!saveParameter("AMCalibData", &_amdata.cfg.calibData, sizeof(Blob::AMCalibData_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando AstData!");
	}
	uint32_t crc = Blob::getCRC32(&_amdata.cfg, sizeof(Blob::AMCfgData_t));
	if(!saveParameter("AMChecksum", &crc, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando Checksum!");
	}
}


//------------------------------------------------------------------------------------
void AMManager::eventMeasureWorkCb() {
	_measure(true);
}


//------------------------------------------------------------------------------------
void AMManager::_measure(bool enable_notif) {

	double value;
	uint32_t multiplier = 1000;

	// obtiene las medidas de energía
	if(_driver->getImportEnergy(&value) == AMDriver::Success){
		_amdata.stat.energyValues.active = value;
	}
	if(_driver->getExportEnergy(&value) == AMDriver::Success){
		_amdata.stat.energyValues.reactive = value;
	}

	// obtiene las medidas instantáneas
	if(_driver->getLineCurrent(&value) == AMDriver::Success){
		_amdata.stat.measureValues.current = value;
	}
	if(_driver->getLineVoltage(&value) == AMDriver::Success){
		_amdata.stat.measureValues.voltage = value;
	}
	if(_driver->getActivePower(&value) == AMDriver::Success){
		_amdata.stat.measureValues.aPow = value;
	}
	if(_driver->getReactivePower(&value) == AMDriver::Success){
		_amdata.stat.measureValues.rPow = value;
	}
	if(_driver->getFrequency(&value) == AMDriver::Success){
		_amdata.stat.measureValues.freq = value;
	}
	if(_driver->getPowerFactor(&value) == AMDriver::Success){
		_amdata.stat.measureValues.pfactor = value;
	}
	if(_driver->getPhase(&value) == AMDriver::Success){
		_amdata.stat.measureValues.phase = value;
	}
	if(_driver->getMeanAparentPower(&value) == AMDriver::Success){
		_amdata.stat.measureValues.msPow = value;
	}

	//DEBUG_TRACE_D(_EXPR_, _MODULE_, "Realizando medida... stat=%x", _amdata.stat.flags);

	// chequeo cada una de las alarmas:
	bool alarm_notif = false;

	// en primer lugar si hay carga activa
	if(_load_data.outValue > 0){
		// chequeo tensión
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.voltage, _amdata.cfg.minmaxData.voltage,
						Blob::AMVoltageOverLimitEvt,
						Blob::AMVoltageBelowLimitEvt,
						Blob::AMVoltageInRangeEvt);
		// chequeo corriente
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.current, _amdata.cfg.minmaxData.current,
						Blob::AMCurrentOverLimitEvt,
						Blob::AMCurrentBelowLimitEvt,
						Blob::AMCurrentInRangeEvt);
		// chequeo phase
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.phase, _amdata.cfg.minmaxData.phase,
						Blob::AMPhaseOverLimitEvt,
						Blob::AMPhaseBelowLimitEvt,
						Blob::AMPhaseInRangeEvt);
		// chequeo factor de potencia
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.pfactor, _amdata.cfg.minmaxData.pfactor,
						Blob::AMPFactorOverLimitEvt,
						Blob::AMPFactorBelowLimitEvt,
						Blob::AMPFactorInRangeEvt);
		// chequeo potencia activa
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.aPow, _amdata.cfg.minmaxData.aPow,
						Blob::AMActPowOverLimitEvt,
						Blob::AMActPowBelowLimitEvt,
						Blob::AMActPowInRangeEvt);
		// chequeo potencia reactiva
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.rPow, _amdata.cfg.minmaxData.rPow,
						Blob::AMReactPowOverLimitEvt,
						Blob::AMReactPowBelowLimitEvt,
						Blob::AMReactPowInRangeEvt);
		// chequeo frecuencia
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.freq, _amdata.cfg.minmaxData.freq,
						Blob::AMFrequencyOverLimitEvt,
						Blob::AMFrequencyBelowLimitEvt,
						Blob::AMFrequencyInRangeEvt);
	}
	//o si no hay carga activa
	else{
		Blob::AMMinMax_t minmax;
		// chequeo corriente
		minmax = {0, _amdata.cfg.minmaxData.current.min, _amdata.cfg.minmaxData.current.thres};
		alarmChecking(	alarm_notif,
						_amdata.stat.measureValues.current, minmax,
						Blob::AMCurrentOverLimitEvt,
						Blob::AMCurrentBelowLimitEvt,
						Blob::AMCurrentInRangeEvt);
	}


	// cada N medidas, envía un evento de medida para no saturar las comunicaciones
	if(--_instant_meas_counter <= 0){
		_instant_meas_counter = _amdata.cfg.measPeriod / (DefaultMeasurePeriod/1000);
		alarm_notif = true;
		_amdata.stat.flags |= (uint32_t)Blob::AMInstantMeasureEvt;
	}
	else{
		_amdata.stat.flags &= ~(uint32_t)Blob::AMInstantMeasureEvt;
	}

	// notifica alarmas
	if(alarm_notif && enable_notif){
		// envía mensaje con los flags que se han activado y que están operativos
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Notificando evento %x", _amdata.stat.flags);
		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
		MBED_ASSERT(pub_topic);
		sprintf(pub_topic, "stat/value/%s", _pub_topic_base);

		if(_json_supported){
			cJSON* jstat = JsonParser::getJsonFromObj(_amdata.stat);
			if(jstat){
				char* jmsg = cJSON_Print(jstat);
				cJSON_Delete(jstat);
				MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
				Heap::memFree(jmsg);
				Heap::memFree(pub_topic);
				return;
			}
		}

		MQ::MQClient::publish(pub_topic, &_amdata.stat, sizeof(Blob::AMStatData_t), &_publicationCb);
		Heap::memFree(pub_topic);
	}
}


//------------------------------------------------------------------------------------
void AMManager::alarmChecking(	bool& alarm_notif,
								double measure,
								Blob::AMMinMax_t data_range,
								uint32_t flag_over_limit,
								uint32_t flag_below_limit,
								uint32_t flag_in_range){

	if(measure > data_range.max && (_amdata.stat.flags & flag_over_limit) == 0){
		_amdata.stat.flags |= flag_over_limit;
		// si el flag está activado, notifica evento
		if(_amdata.cfg.evtFlagMask & flag_over_limit){
			alarm_notif = true;
		}
	}
	else if(measure < data_range.min && (_amdata.stat.flags & flag_below_limit) == 0){
		_amdata.stat.flags |= flag_below_limit;
		// si el flag está activado, notifica evento
		if(_amdata.cfg.evtFlagMask & flag_below_limit){
			alarm_notif = true;
		}
	}
	else if((measure >= (data_range.min + data_range.thres) && measure <= (data_range.max - data_range.thres)) && (_amdata.stat.flags & (flag_over_limit|flag_below_limit)) != 0){
		_amdata.stat.flags &= ~(flag_over_limit|flag_below_limit);
		//_amdata.stat.flags |= flag_in_range;

		// si el flag está activado, notifica evento
		if(_amdata.cfg.evtFlagMask & flag_in_range){
			alarm_notif = true;
		}
	}
}


//------------------------------------------------------------------------------------
void AMManager::_updateConfig(const Blob::AMCfgData_t& cfg, uint32_t keys, Blob::ErrorData_t& err){
	if(keys == Blob::AMKeyNone){
		err.code = Blob::ErrEmptyContent;
		goto _updateConfigExit;
	}
	#warning TODO : Verificar datos entrantes para generar errores si son datos incorrectos
	if(keys & Blob::AMKeyCfgUpd){
		_amdata.cfg.updFlagMask = cfg.updFlagMask;
	}
	if(keys & Blob::AMKeyCfgEvt){
		_amdata.cfg.evtFlagMask = cfg.evtFlagMask;
	}
	if(keys & Blob::AMKeyCfgMeas){
		_amdata.cfg.measPeriod = cfg.measPeriod;
	}
	if(keys & Blob::AMKeyCfgMnxVolt){
		_amdata.cfg.minmaxData.voltage = cfg.minmaxData.voltage;
	}
	if(keys & Blob::AMKeyCfgMnxCurr){
		_amdata.cfg.minmaxData.current = cfg.minmaxData.current;
	}
	if(keys & Blob::AMKeyCfgMnxPhase){
		_amdata.cfg.minmaxData.phase = cfg.minmaxData.phase;
	}
	if(keys & Blob::AMKeyCfgMnxPfact){
		_amdata.cfg.minmaxData.pfactor = cfg.minmaxData.pfactor;
	}
	if(keys & Blob::AMKeyCfgMnxApow){
		_amdata.cfg.minmaxData.aPow = cfg.minmaxData.aPow;
	}
	if(keys & Blob::AMKeyCfgMnxRpow){
		_amdata.cfg.minmaxData.rPow = cfg.minmaxData.rPow;
	}
	if(keys & Blob::AMKeyCfgMnxMpow){
		_amdata.cfg.minmaxData.msPow = cfg.minmaxData.msPow;
	}
	if(keys & Blob::AMKeyCfgMnxFreq){
		_amdata.cfg.minmaxData.freq = cfg.minmaxData.freq;
	}
	if(keys & Blob::AMKeyCfgCalMetr){
		for(int i=0;i<Blob::AMCalibRegCount;i++)
			_amdata.cfg.calibData.meterRegs[i] = cfg.calibData.meterRegs[i];
	}
	if(keys & Blob::AMKeyCfgCalMea){
		for(int i=0;i<Blob::AMCalibRegCount;i++)
			_amdata.cfg.calibData.measRegs[i] = cfg.calibData.measRegs[i];
	}
_updateConfigExit:
	strcpy(err.descr, Blob::errList[err.code]);
}




//------------------------------------------------------------------------------------
//-- JSON SUPPORT --------------------------------------------------------------------
//------------------------------------------------------------------------------------


namespace JSON{


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMCfg(const Blob::AMCfgData_t& cfg){
	cJSON* energy = NULL;

	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}
	cJSON *minmaxData = NULL;
	cJSON *calibData = NULL;
	cJSON *regs = NULL;
	cJSON *value = NULL;
	// key: energy.updFlags
	cJSON_AddNumberToObject(energy, JsonParser::p_updFlags, cfg.updFlagMask);

	// key: energy.evtFlags
	cJSON_AddNumberToObject(energy, JsonParser::p_evtFlags, cfg.evtFlagMask);

	// key: energy.measPeriod
	cJSON_AddNumberToObject(energy, JsonParser::p_measPeriod, cfg.measPeriod);

	// key: minmaxData
	if((minmaxData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}

	// key: minmaxData.voltage
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.voltage.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.voltage.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.voltage.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_voltage, value);

	// key: minmaxData.current
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.current.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.current.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.current.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_current, value);

	// key: minmaxData.phase
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.phase.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.phase.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.phase.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_phase, value);

	// key: minmaxData.pfactor
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.pfactor.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.pfactor.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.pfactor.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_pfactor, value);

	// key: minmaxData.aPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.aPow.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.aPow.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.aPow.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_aPow, value);

	// key: minmaxData.rPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.rPow.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.rPow.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.rPow.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_rPow, value);

	// key: minmaxData.msPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.msPow.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.msPow.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.msPow.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_msPow, value);

	// key: minmaxData.freq
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.freq.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.freq.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.freq.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_freq, value);

	cJSON_AddItemToObject(energy, JsonParser::p_minmaxData, minmaxData);

	// key: calibData
	if((calibData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}

	// key: calibData.meterRegs
	if((regs=cJSON_CreateArray()) == NULL){
		cJSON_Delete(calibData);
		cJSON_Delete(energy);
		return NULL;
	}
	for(int i=0; i < Blob::AMCalibRegCount; i++){
		if((value = cJSON_CreateNumber(cfg.calibData.meterRegs[i])) == NULL){
			cJSON_Delete(regs);
			cJSON_Delete(calibData);
			cJSON_Delete(energy);
			return NULL;
		}
		cJSON_AddItemToArray(regs, value);
	}
	cJSON_AddItemToObject(calibData, JsonParser::p_meterRegs, regs);

	// key: calibData.measRegs
	if((regs=cJSON_CreateArray()) == NULL){
		cJSON_Delete(calibData);
		cJSON_Delete(energy);
		return NULL;
	}
	for(int i=0; i < Blob::AMCalibRegCount; i++){
		if((value = cJSON_CreateNumber(cfg.calibData.measRegs[i])) == NULL){
			cJSON_Delete(regs);
			cJSON_Delete(calibData);
			cJSON_Delete(energy);
			return NULL;
		}
		cJSON_AddItemToArray(regs, value);
	}
	cJSON_AddItemToObject(calibData, JsonParser::p_measRegs, regs);
	cJSON_AddItemToObject(energy, JsonParser::p_calibData, calibData);
	return energy;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMStat(const Blob::AMStatData_t& stat) {
	cJSON* energy = NULL;
	cJSON* value = NULL;

	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// key: flags
	cJSON_AddNumberToObject(energy, JsonParser::p_flags, stat.flags);

	// key: energyValues
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_active, stat.energyValues.active);
	cJSON_AddNumberToObject(value, JsonParser::p_reactive, stat.energyValues.reactive);
	cJSON_AddItemToObject(energy, JsonParser::p_energyValues, value);

	// key: measureValues
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_voltage, stat.measureValues.voltage);
	cJSON_AddNumberToObject(value, JsonParser::p_current, stat.measureValues.current);
	cJSON_AddNumberToObject(value, JsonParser::p_phase, stat.measureValues.phase);
	cJSON_AddNumberToObject(value, JsonParser::p_pfactor, stat.measureValues.pfactor);
	cJSON_AddNumberToObject(value, JsonParser::p_aPow, stat.measureValues.aPow);
	cJSON_AddNumberToObject(value, JsonParser::p_rPow, stat.measureValues.rPow);
	cJSON_AddNumberToObject(value, JsonParser::p_msPow, stat.measureValues.msPow);
	cJSON_AddNumberToObject(value, JsonParser::p_freq, stat.measureValues.freq);
	cJSON_AddItemToObject(energy, JsonParser::p_measureValues, value);
	return energy;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMBoot(const Blob::AMBootData_t& boot){
	cJSON* energy = NULL;
	cJSON* item = NULL;
	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	if((item = getJsonFromAMCfg(boot.cfg)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(energy, JsonParser::p_cfg, item);

	if((item = getJsonFromAMStat(boot.stat)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(energy, JsonParser::p_stat, item);
	return energy;

__encodeBoot_Err:
	cJSON_Delete(energy);
	return NULL;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMLoad(const Blob::AMLoadData_t& data) {
	cJSON* obj = NULL;
	cJSON* value = NULL;

	if((obj=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// key: flags
	cJSON_AddNumberToObject(obj, JsonParser::p_outValue, data.outValue);
	return obj;
}


//------------------------------------------------------------------------------------
uint32_t getAMCfgFromJson(Blob::AMCfgData_t &cfg, cJSON* json){
	cJSON* obj = NULL;
	cJSON* minmaxData = NULL;
	cJSON* calibData = NULL;
	cJSON* array = NULL;
	cJSON* value = NULL;
	uint32_t keys = 0;

	if(json == NULL){
		return 0;
	}

	if((obj = cJSON_GetObjectItem(json,JsonParser::p_updFlags)) != NULL){
		cfg.updFlagMask = (Blob::AMUpdFlags)obj->valueint;
		keys |= Blob::AMKeyCfgUpd;
	}
	if((obj = cJSON_GetObjectItem(json,JsonParser::p_evtFlags)) != NULL){
		cfg.evtFlagMask = (Blob::AMEvtFlags)obj->valueint;
		keys |= Blob::AMKeyCfgEvt;
	}
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_measPeriod)) != NULL){
		cfg.measPeriod = (uint32_t)obj->valueint;
		keys |= Blob::AMKeyCfgMeas;
	}

	//key: minmaxData
	if((minmaxData = cJSON_GetObjectItem(json, JsonParser::p_minmaxData)) != NULL){
		//key: minmaxData.voltage
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_voltage)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.voltage.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.voltage.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value,JsonParser::p_thres)) != NULL){
				cfg.minmaxData.voltage.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxVolt;
		}

		//key: minmaxData.current
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_current)) != NULL){
			if((obj = cJSON_GetObjectItem(value,JsonParser::p_min)) != NULL){
				cfg.minmaxData.current.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.current.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.current.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxCurr;
		}

		//key: minmaxData.phase
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_phase)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.phase.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.phase.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.phase.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxPhase;
		}

		//key: minmaxData.pfactor
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_pfactor)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.pfactor.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.pfactor.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.pfactor.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxPfact;
		}

		//key: minmaxData.aPow
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_aPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.aPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.aPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.aPow.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxApow;
		}

		//key: minmaxData.rPow
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_rPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.rPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.rPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.rPow.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxRpow;
		}

		//key: minmaxData.msPow
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_msPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.msPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.msPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.msPow.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxMpow;
		}

		//key: minmaxData.freq
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_freq)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.freq.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.freq.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.freq.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxFreq;
		}

	}

	if((calibData = cJSON_GetObjectItem(json, JsonParser::p_calibData)) != NULL){
		if((array = cJSON_GetObjectItem(calibData, JsonParser::p_meterRegs)) != NULL){
			if(cJSON_GetArraySize(array) <= Blob::AMCalibRegCount){
				for(int i=0;i<cJSON_GetArraySize(array);i++){
					value = cJSON_GetArrayItem(array, i);
					cfg.calibData.meterRegs[i] = value->valueint;
				}
				keys |= Blob::AMKeyCfgCalMetr;
			}
		}
		if((array = cJSON_GetObjectItem(calibData, JsonParser::p_measRegs)) != NULL){
			if(cJSON_GetArraySize(array) <= Blob::AMCalibRegCount){
				for(int i=0;i<cJSON_GetArraySize(array);i++){
					value = cJSON_GetArrayItem(array, i);
					cfg.calibData.measRegs[i] = value->valueint;
				}
				keys |= Blob::AMKeyCfgCalMea;
			}
		}
	}

	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getAMStatFromJson(Blob::AMStatData_t &stat, cJSON* json) {
	cJSON* obj = NULL;
	cJSON* minmaxData = NULL;
	cJSON* calibData = NULL;
	cJSON* array = NULL;
	cJSON* value = NULL;
	uint32_t keys = 0;

	if(json == NULL){
		return 0;
	}

	if((obj = cJSON_GetObjectItem(json,JsonParser::p_flags)) == NULL){
		return 0;
	}
	stat.flags = (Blob::AMUpdFlags)obj->valueint;

	//key: energyValues
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_energyValues)) == NULL){
		return 0;
	}
	//key: active
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_active)) == NULL){
		return 0;
	}
	stat.energyValues.active = value->valueint;
	//key: reactive
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_reactive)) == NULL){
		return 0;
	}
	stat.energyValues.reactive = value->valueint;

	//key: measureValues
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_measureValues)) == NULL){
		return 0;
	}
	//key: current
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_current)) == NULL){
		return 0;
	}
	stat.measureValues.current = value->valuedouble;
	//key: voltage
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_voltage)) == NULL){
		return 0;
	}
	stat.measureValues.voltage = value->valuedouble;
	//key: freq
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_freq)) == NULL){
		return 0;
	}
	stat.measureValues.freq = value->valuedouble;
	//key: pfactor
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_pfactor)) == NULL){
		return 0;
	}
	stat.measureValues.pfactor = value->valuedouble;
	//key: phase
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_phase)) == NULL){
		return 0;
	}
	stat.measureValues.phase = value->valuedouble;
	//key: aPow
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_aPow)) == NULL){
		return 0;
	}
	stat.measureValues.aPow = value->valuedouble;
	//key: rPow
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_rPow)) == NULL){
		return 0;
	}
	stat.measureValues.rPow = value->valuedouble;
	//key: msPow
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_msPow)) == NULL){
		return 0;
	}
	stat.measureValues.msPow = value->valuedouble;
	return 1;
}


//------------------------------------------------------------------------------------
uint32_t getAMBootFromJson(Blob::AMBootData_t &obj, cJSON* json){
	cJSON* cfg = NULL;
	cJSON* stat = NULL;
	uint32_t keys = 0;

	if(json == NULL){
		return 0;
	}
	if((cfg = cJSON_GetObjectItem(json, JsonParser::p_cfg)) == NULL){
		return 0;
	}
	keys |= getAMCfgFromJson(obj.cfg, cfg);

	if((stat = cJSON_GetObjectItem(json, JsonParser::p_stat)) == NULL){
		return 0;
	}
	keys |= getAMStatFromJson(obj.stat, stat);
	return keys;
}




//------------------------------------------------------------------------------------
uint32_t getAMLoadFromJson(Blob::AMLoadData_t &data, cJSON* json){
	cJSON *obj = NULL;
	if(json == NULL){
		return 0;
	}

	// key: outValue
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_outValue)) == NULL){
		return 0;
	}

	data.outValue = obj->valueint;
	return 1;
}

}	// end namespace JSON

