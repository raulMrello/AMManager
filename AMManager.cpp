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
osEvent AMManager:: getOsEvent(){
	return _queue.get();
}



//------------------------------------------------------------------------------------
void AMManager::publicationCb(const char* topic, int32_t result){

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

	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Realizando medida... stat=%x", _amdata.stat.flags);

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




