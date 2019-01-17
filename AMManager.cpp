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

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(msg_len != sizeof(Blob::SetRequest_t<Blob::AMCfgData_t>)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::AMCfgData_t
        Blob::SetRequest_t<Blob::AMCfgData_t>* req = (Blob::SetRequest_t<Blob::AMCfgData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::AMCfgData_t>));
        MBED_ASSERT(req);
        *req = *((Blob::SetRequest_t<Blob::AMCfgData_t>*)msg);
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

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
		Blob::GetRequest_t* req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
		MBED_ASSERT(req);
		*req = *((Blob::GetRequest_t*)msg);
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

    // si es un comando para solicitar el estado
    if(MQ::MQClient::isTokenRoot(topic, "get/value")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
		Blob::GetRequest_t* req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
		MBED_ASSERT(req);
		*req = *((Blob::GetRequest_t*)msg);
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

    // si es un comando para notificar un cambio de estado en la carga del módulo de medida...
    if(MQ::MQClient::isTokenRoot(topic, "set/load")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(msg_len != sizeof(Blob::AMLoadData_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::AMCfgData_t
        Blob::AMLoadData_t* req = (Blob::AMLoadData_t*)Heap::memAlloc(sizeof(Blob::AMLoadData_t));
        MBED_ASSERT(req);
        *req = *((Blob::AMLoadData_t*)msg);
        op->sig = RecvLoadSet;
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
        	// si no hay errores, actualiza la configuración
        	if(req->_error.code == Blob::ErrOK){
				_updateConfig(req->data, req->_error);
        	}
        	// si hay errores en el mensaje o en la actualización, devuelve resultado sin hacer nada
        	if(req->_error.code != Blob::ErrOK){
        		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<Blob::AMCfgData_t>* resp = new Blob::Response_t<Blob::AMCfgData_t>(req->idTrans, req->_error, _amdata.cfg);
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
				char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<Blob::AMCfgData_t>* resp = new Blob::Response_t<Blob::AMCfgData_t>(req->idTrans, req->_error, _amdata.cfg);
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
	// borra los parámetros no recuperados
	_amdata.cfg.keys = Blob::AMKeyNone;

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
	// borra los parámetros no recuperados
	_amdata.cfg.keys = Blob::AMKeyNone;
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
void AMManager::_updateConfig(const Blob::AMCfgData_t& cfg, Blob::ErrorData_t& err){
	if(cfg.keys == Blob::AMKeyNone){
		err.code = Blob::ErrEmptyContent;
		goto _updateConfigExit;
	}
	#warning TODO : Verificar datos entrantes para generar errores si son datos incorrectos
	if(cfg.keys & Blob::AMKeyCfgUpd){
		_amdata.cfg.updFlagMask = cfg.updFlagMask;
	}
	if(cfg.keys & Blob::AMKeyCfgEvt){
		_amdata.cfg.evtFlagMask = cfg.evtFlagMask;
	}
	if(cfg.keys & Blob::AMKeyCfgMeas){
		_amdata.cfg.measPeriod = cfg.measPeriod;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxVolt){
		_amdata.cfg.minmaxData.voltage = cfg.minmaxData.voltage;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxCurr){
		_amdata.cfg.minmaxData.current = cfg.minmaxData.current;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxPhase){
		_amdata.cfg.minmaxData.phase = cfg.minmaxData.phase;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxPfact){
		_amdata.cfg.minmaxData.pfactor = cfg.minmaxData.pfactor;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxApow){
		_amdata.cfg.minmaxData.aPow = cfg.minmaxData.aPow;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxRpow){
		_amdata.cfg.minmaxData.rPow = cfg.minmaxData.rPow;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxMpow){
		_amdata.cfg.minmaxData.msPow = cfg.minmaxData.msPow;
	}
	if(cfg.keys & Blob::AMKeyCfgMnxFreq){
		_amdata.cfg.minmaxData.freq = cfg.minmaxData.freq;
	}
	if(cfg.keys & Blob::AMKeyCfgCalMetr){
		for(int i=0;i<Blob::AMCalibRegCount;i++)
			_amdata.cfg.calibData.meterRegs[i] = cfg.calibData.meterRegs[i];
	}
	if(cfg.keys & Blob::AMKeyCfgCalMea){
		for(int i=0;i<Blob::AMCalibRegCount;i++)
			_amdata.cfg.calibData.measRegs[i] = cfg.calibData.measRegs[i];
	}
_updateConfigExit:
	strcpy(err.descr, Blob::errList[err.code]);
}


