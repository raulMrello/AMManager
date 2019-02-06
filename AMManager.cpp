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
			if(!(json_decoded = decodeSetRequest(*req, (char*)msg))){
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
			if(!(json_decoded = JSON::decodeGetRequest(*req, (char*)msg))){
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
			if(!(json_decoded = JSON::decodeGetRequest(*req, (char*)msg))){
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
			if(!(json_decoded = _decodeLoadData(*load, (char*)msg))){
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
					cJSON* jresp = encodeCfgResponse(*resp);
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
					cJSON* jresp = encodeCfgResponse(*resp);
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
				cJSON* jresp = encodeCfgResponse(*resp);
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
				cJSON* jresp = encodeStatResponse(*resp);
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
				cJSON* jboot = _encodeBoot();
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
			cJSON* jstat = _encodeStat();
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


/** JSON keys */
static const char* p_active			= "active";
static const char* p_aPow			= "aPow";
static const char* p_energy 		= "energy";
static const char* p_calibData		= "calibData";
static const char* p_cfg 			= "cfg";
static const char* p_current		= "current";
static const char* p_stat	 		= "stat";
static const char* p_code	 		= "code";
static const char* p_descr	 		= "descr";
static const char* p_energyValues	= "energyValues";
static const char* p_error	 		= "error";
static const char* p_evtFlags 		= "evtFlags";
static const char* p_flags			= "flags";
static const char* p_freq			= "freq";
static const char* p_header			= "header";
static const char* p_idTrans 		= "idTrans";
static const char* p_max			= "max";
static const char* p_measPeriod		= "measPeriod";
static const char* p_measRegs		= "measRegs";
static const char* p_measureValues	= "measureValues";
static const char* p_meterRegs		= "meterRegs";
static const char* p_min			= "min";
static const char* p_minmaxData		= "minmaxData";
static const char* p_msPow			= "msPow";
static const char* p_outValue		= "outValue";
static const char* p_pfactor		= "pfactor";
static const char* p_phase			= "phase";
static const char* p_reactive		= "reactive";
static const char* p_rPow			= "rPow";
static const char* p_thres			= "thres";
static const char* p_timestamp		= "timestamp";
static const char* p_updFlags 		= "updFlags";
static const char* p_voltage 		= "voltage";


//------------------------------------------------------------------------------------
cJSON* AMManager::encodeCfg(const Blob::AMCfgData_t& cfg){
	cJSON* energy = NULL;

	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}
	cJSON *minmaxData = NULL;
	cJSON *calibData = NULL;
	cJSON *regs = NULL;
	cJSON *value = NULL;
	// key: energy.updFlags
	cJSON_AddNumberToObject(energy, p_updFlags, cfg.updFlagMask);

	// key: energy.evtFlags
	cJSON_AddNumberToObject(energy, p_evtFlags, cfg.evtFlagMask);

	// key: energy.measPeriod
	cJSON_AddNumberToObject(energy, p_measPeriod, cfg.measPeriod);

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
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.voltage.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.voltage.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.voltage.thres);
	cJSON_AddItemToObject(minmaxData, p_voltage, value);

	// key: minmaxData.current
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.current.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.current.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.current.thres);
	cJSON_AddItemToObject(minmaxData, p_current, value);

	// key: minmaxData.phase
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.phase.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.phase.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.phase.thres);
	cJSON_AddItemToObject(minmaxData, p_phase, value);

	// key: minmaxData.pfactor
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.pfactor.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.pfactor.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.pfactor.thres);
	cJSON_AddItemToObject(minmaxData, p_pfactor, value);

	// key: minmaxData.aPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.aPow.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.aPow.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.aPow.thres);
	cJSON_AddItemToObject(minmaxData, p_aPow, value);

	// key: minmaxData.rPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.rPow.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.rPow.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.rPow.thres);
	cJSON_AddItemToObject(minmaxData, p_rPow, value);

	// key: minmaxData.msPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.msPow.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.msPow.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.msPow.thres);
	cJSON_AddItemToObject(minmaxData, p_msPow, value);

	// key: minmaxData.freq
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.minmaxData.freq.min);
	cJSON_AddNumberToObject(value, p_max, cfg.minmaxData.freq.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.minmaxData.freq.thres);
	cJSON_AddItemToObject(minmaxData, p_freq, value);

	cJSON_AddItemToObject(energy, p_minmaxData, minmaxData);

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
	cJSON_AddItemToObject(calibData, p_meterRegs, regs);

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
	cJSON_AddItemToObject(calibData, p_measRegs, regs);
	cJSON_AddItemToObject(energy, p_calibData, calibData);
	return energy;
}


//------------------------------------------------------------------------------------
cJSON* AMManager::encodeStat(const Blob::AMStatData_t& stat){
	cJSON* energy = NULL;
	cJSON* value = NULL;

	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// key: flags
	cJSON_AddNumberToObject(energy, p_flags, stat.flags);

	// key: energyValues
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_active, stat.energyValues.active);
	cJSON_AddNumberToObject(value, p_reactive, stat.energyValues.reactive);
	cJSON_AddItemToObject(energy, p_energyValues, value);

	// key: measureValues
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_voltage, stat.measureValues.voltage);
	cJSON_AddNumberToObject(value, p_current, stat.measureValues.current);
	cJSON_AddNumberToObject(value, p_phase, stat.measureValues.phase);
	cJSON_AddNumberToObject(value, p_pfactor, stat.measureValues.pfactor);
	cJSON_AddNumberToObject(value, p_aPow, stat.measureValues.aPow);
	cJSON_AddNumberToObject(value, p_rPow, stat.measureValues.rPow);
	cJSON_AddNumberToObject(value, p_msPow, stat.measureValues.msPow);
	cJSON_AddNumberToObject(value, p_freq, stat.measureValues.freq);
	cJSON_AddItemToObject(energy, p_measureValues, value);
	return energy;
}


//------------------------------------------------------------------------------------
bool AMManager::decodeSetRequest(Blob::SetRequest_t<Blob::AMCfgData_t>&req, char* json_data){
	cJSON *obj = NULL;
	cJSON *value = NULL;
	cJSON *array = NULL;
	cJSON *energy = NULL;
	cJSON *minmaxData = NULL;
	cJSON *calibData = NULL;
	cJSON *root = NULL;
	req.keys = Blob::AMKeyNone;
	req._error.code = Blob::ErrOK;
	strcpy(req._error.descr, Blob::errList[req._error.code]);

	if((root = cJSON_Parse(json_data)) == NULL){
		req._error.code = Blob::ErrJsonMalformed;
		req.idTrans = Blob::UnusedIdTrans;
		goto __decode_null_exitAMCfg;
	}

	// key: idTrans
	if((obj = cJSON_GetObjectItem(root, p_idTrans)) == NULL){
		req._error.code = Blob::ErrIdTransInvalid;
		req.idTrans = Blob::UnusedIdTrans;
		goto __decode_null_exitAMCfg;
	}
	req.idTrans = obj->valueint;

	//key: energy
	if((energy = cJSON_GetObjectItem(root, p_energy)) == NULL){
		req._error.code = Blob::ErrEnergyMissing;
		goto __decode_null_exitAMCfg;
	}

	if((obj = cJSON_GetObjectItem(energy, p_updFlags)) != NULL){
		req.data.updFlagMask = (Blob::AMUpdFlags)obj->valueint;
		req.keys |= Blob::AMKeyCfgUpd;
	}
	if((obj = cJSON_GetObjectItem(energy, p_evtFlags)) != NULL){
		req.data.evtFlagMask = (Blob::AMEvtFlags)obj->valueint;
		req.keys |= Blob::AMKeyCfgEvt;
	}
	if((obj = cJSON_GetObjectItem(energy, p_measPeriod)) != NULL){
		req.data.measPeriod = (uint32_t)obj->valueint;
		req.keys |= Blob::AMKeyCfgMeas;
	}

	//key: minmaxData
	if((minmaxData = cJSON_GetObjectItem(energy, p_minmaxData)) != NULL){
		//key: minmaxData.voltage
		if((value = cJSON_GetObjectItem(minmaxData, p_voltage)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_min)) != NULL){
				req.data.minmaxData.voltage.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.voltage.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value,p_thres)) != NULL){
				req.data.minmaxData.voltage.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxVolt;
		}

		//key: minmaxData.current
		if((value = cJSON_GetObjectItem(minmaxData, p_current)) != NULL){
			if((obj = cJSON_GetObjectItem(value,p_min)) != NULL){
				req.data.minmaxData.current.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.current.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_thres)) != NULL){
				req.data.minmaxData.current.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxCurr;
		}

		//key: minmaxData.phase
		if((value = cJSON_GetObjectItem(minmaxData, p_phase)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_min)) != NULL){
				req.data.minmaxData.phase.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.phase.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_thres)) != NULL){
				req.data.minmaxData.phase.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxPhase;
		}

		//key: minmaxData.pfactor
		if((value = cJSON_GetObjectItem(minmaxData, p_pfactor)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_min)) != NULL){
				req.data.minmaxData.pfactor.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.pfactor.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_thres)) != NULL){
				req.data.minmaxData.pfactor.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxPfact;
		}

		//key: minmaxData.aPow
		if((value = cJSON_GetObjectItem(minmaxData, p_aPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_min)) != NULL){
				req.data.minmaxData.aPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.aPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_thres)) != NULL){
				req.data.minmaxData.aPow.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxApow;
		}

		//key: minmaxData.rPow
		if((value = cJSON_GetObjectItem(minmaxData, p_rPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_min)) != NULL){
				req.data.minmaxData.rPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.rPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_thres)) != NULL){
				req.data.minmaxData.rPow.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxRpow;
		}

		//key: minmaxData.msPow
		if((value = cJSON_GetObjectItem(minmaxData, p_msPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_min)) != NULL){
				req.data.minmaxData.msPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.msPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_thres)) != NULL){
				req.data.minmaxData.msPow.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxMpow;
		}

		//key: minmaxData.freq
		if((value = cJSON_GetObjectItem(minmaxData, p_freq)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_min)) != NULL){
				req.data.minmaxData.freq.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_max)) != NULL){
				req.data.minmaxData.freq.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, p_thres)) != NULL){
				req.data.minmaxData.freq.thres = obj->valuedouble;
			}
			req.keys |= Blob::AMKeyCfgMnxFreq;
		}

	}

	if((calibData = cJSON_GetObjectItem(energy, p_calibData)) != NULL){
		if((array = cJSON_GetObjectItem(calibData, p_meterRegs)) != NULL){
			if(cJSON_GetArraySize(array) <= Blob::AMCalibRegCount){
				for(int i=0;i<cJSON_GetArraySize(array);i++){
					value = cJSON_GetArrayItem(array, i);
					req.data.calibData.meterRegs[i] = value->valueint;
				}
				req.keys |= Blob::AMKeyCfgCalMetr;
			}
		}
		if((array = cJSON_GetObjectItem(calibData, p_measRegs)) != NULL){
			if(cJSON_GetArraySize(array) <= Blob::AMCalibRegCount){
				for(int i=0;i<cJSON_GetArraySize(array);i++){
					value = cJSON_GetArrayItem(array, i);
					req.data.calibData.measRegs[i] = value->valueint;
				}
				req.keys |= Blob::AMKeyCfgCalMea;
			}
		}
	}

__decode_null_exitAMCfg:
	strcpy(req._error.descr, Blob::errList[req._error.code]);
	if(root){
		cJSON_Delete(root);
	}
	if(req._error.code == Blob::ErrOK){
		return true;
	}
	return false;
}


//------------------------------------------------------------------------------------
cJSON* AMManager::encodeCfgResponse(const Blob::Response_t<Blob::AMCfgData_t> &resp){
	// keys: root, idtrans, header, error, astcal
	cJSON *header = NULL;
	cJSON *error = NULL;
	cJSON *energy = NULL;
	cJSON *value = NULL;
	cJSON *root = cJSON_CreateObject();

	if(!root){
		return NULL;
	}

	// key: idTrans sólo se envía si está en uso
	if(resp.idTrans != Blob::UnusedIdTrans){
		cJSON_AddNumberToObject(root, p_idTrans, resp.idTrans);
	}

	// key: header
	if((header=cJSON_CreateObject()) == NULL){
		goto _parseAMCfg_Exit;
	}
	cJSON_AddNumberToObject(header, p_timestamp, resp.header.timestamp);
	cJSON_AddItemToObject(root, p_header, header);

	// key: error sólo se envía si el error es distinto de Blob::ErrOK
	if(resp.error.code != Blob::ErrOK){
		if((error=cJSON_CreateObject()) == NULL){
			goto _parseAMCfg_Exit;
		}
		cJSON_AddNumberToObject(error, p_code, resp.error.code);
		if((value=cJSON_CreateString(resp.error.descr)) == NULL){
			goto _parseAMCfg_Exit;
		}
		cJSON_AddItemToObject(error, p_descr, value);
		// añade objeto
		cJSON_AddItemToObject(root, p_error, error);
	}

	// key: astcal sólo se envía si el error es Blob::ErrOK
	if(resp.error.code == Blob::ErrOK){
		if((energy = encodeCfg(resp.data)) == NULL){
			goto _parseAMCfg_Exit;
		}
		// añade objeto
		cJSON_AddItemToObject(root, p_energy, energy);
	}
	return root;

_parseAMCfg_Exit:
	cJSON_Delete(root);
	return NULL;
}


//------------------------------------------------------------------------------------
cJSON* AMManager::encodeStatResponse(const Blob::Response_t<Blob::AMStatData_t> &resp){
	// keys: root, idtrans, header, error, energy
	cJSON *header = NULL;
	cJSON *error = NULL;
	cJSON *energy = NULL;
	cJSON *value = NULL;
	cJSON *root = cJSON_CreateObject();

	if(!root){
		return NULL;
	}

	// key: idTrans sólo se envía si está en uso
	if(resp.idTrans != Blob::UnusedIdTrans){
		cJSON_AddNumberToObject(root, p_idTrans, resp.idTrans);
	}

	// key: header
	if((header=cJSON_CreateObject()) == NULL){
		goto _parseAMStat_Exit;
	}
	cJSON_AddNumberToObject(header, p_timestamp, resp.header.timestamp);
	cJSON_AddItemToObject(root, p_header, header);

	// key: error sólo se envía si el error es distinto de Blob::ErrOK
	if(resp.error.code != Blob::ErrOK){
		if((error=cJSON_CreateObject()) == NULL){
			goto _parseAMStat_Exit;
		}
		cJSON_AddNumberToObject(error, p_code, resp.error.code);
		if((value=cJSON_CreateString(resp.error.descr)) == NULL){
			goto _parseAMStat_Exit;
		}
		cJSON_AddItemToObject(error, p_descr, value);
		// añade objeto
		cJSON_AddItemToObject(root, p_error, error);
	}

	// key: astcal sólo se envía si el error es Blob::ErrOK
	if(resp.error.code == Blob::ErrOK){
		if((energy = encodeStat(resp.data)) == NULL){
			goto _parseAMStat_Exit;
		}
		// añade objeto
		cJSON_AddItemToObject(root, p_energy, energy);
	}
	return root;

_parseAMStat_Exit:
	cJSON_Delete(root);
	return NULL;
}


//------------------------------------------------------------------------------------
cJSON* AMManager::encodeBoot(const Blob::AMBootData_t& boot){
	cJSON* energy = NULL;
	cJSON* item = NULL;
	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	if((item = encodeCfg(boot.cfg)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(energy, p_cfg, item);

	if((item = encodeStat(boot.stat)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(energy, p_stat, item);
	return energy;

__encodeBoot_Err:
	cJSON_Delete(energy);
	return NULL;
}


//------------------------------------------------------------------------------------
bool AMManager::_decodeLoadData(Blob::AMLoadData_t& result, const char* json){
	cJSON *root = NULL;
	cJSON *obj = NULL;
	if((root = cJSON_Parse(json)) == NULL){
		return false;
	}

	// key: outValue
	if((obj = cJSON_GetObjectItem(root, p_outValue)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	result.outValue = obj->valueint;
	cJSON_Delete(root);
	return true;
}


