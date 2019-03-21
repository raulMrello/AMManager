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
#define _EXPR_	(!IS_ISR())


 
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
AMManager::AMManager(AMDriver* driver, FSManager* fs, bool defdbg, const char* name) : ActiveModule(name, osPriorityNormal, 4096, fs, defdbg), _name(name) {

	// Establece el soporte de JSON
	_json_supported = false;
	#if AMMANAGER_ENABLE_JSON_SUPPORT == 1
	_json_supported = true;
	#endif

    if(defdbg){
    	esp_log_level_set(_MODULE_, ESP_LOG_DEBUG);
    }
    else{
    	esp_log_level_set(_MODULE_, ESP_LOG_WARN);
    }

	// referencia al driver registrado
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

	AMDriver::ElectricParams eparam;
	uint32_t keys;


	// lee todos los parámetros eléctricos de cada analizador
	for(int i=0; i<_amdata._numAnalyzers; i++){
		if(_driver->getElectricParams(eparam, keys, i) == AMDriver::Success){
			// visualiza los parámetros leídos
			if(keys & AMDriver::ElecKey_Voltage){
				_amdata.analyzers[i].stat.measureValues.voltage = eparam.voltage;
			}
			if(keys & AMDriver::ElecKey_Current){
				_amdata.analyzers[i].stat.measureValues.current = eparam.current;
			}
			if(keys & AMDriver::ElecKey_ActivePow){
				_amdata.analyzers[i].stat.measureValues.aPow = eparam.aPow;
			}
			if(keys & AMDriver::ElecKey_ReactivePow){
				_amdata.analyzers[i].stat.measureValues.rPow = eparam.rPow;
			}
			if(keys & AMDriver::ElecKey_ApparentPow){
				_amdata.analyzers[i].stat.measureValues.msPow = eparam.mPow;
			}
			if(keys & AMDriver::ElecKey_PowFactor){
				_amdata.analyzers[i].stat.measureValues.pfactor = eparam.pFactor;
			}
			if(keys & AMDriver::ElecKey_THDAmpere){
				_amdata.analyzers[i].stat.measureValues.thdA = eparam.thdAmp;
			}
			if(keys & AMDriver::ElecKey_THDVoltage){
				_amdata.analyzers[i].stat.measureValues.thdV = eparam.thdVolt;
			}
			if(keys & AMDriver::ElecKey_Frequency){
				_amdata.analyzers[i].stat.measureValues.freq = eparam.freq;
			}
			if(keys & AMDriver::ElecKey_ActiveEnergy){
				_amdata.analyzers[i].stat.energyValues.active = eparam.aEnergy;
			}
			if(keys & AMDriver::ElecKey_ReactiveEnergy){
				_amdata.analyzers[i].stat.energyValues.reactive = eparam.rEnergy;
			}
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Realizando medida... stat=%x", _amdata.analyzers[i].stat.flags);

		// chequeo cada una de las alarmas:
		bool alarm_notif = false;

		// en primer lugar si hay carga activa
		if(_load_data[i] > 0){
			// chequeo tensión
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.voltage, _amdata.cfg.minmaxData.voltage,
							Blob::AMVoltageOverLimitEvt,
							Blob::AMVoltageBelowLimitEvt,
							Blob::AMVoltageInRangeEvt);
			// chequeo corriente
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.current, _amdata.cfg.minmaxData.current,
							Blob::AMCurrentOverLimitEvt,
							Blob::AMCurrentBelowLimitEvt,
							Blob::AMCurrentInRangeEvt);
			// chequeo phase
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.phase, _amdata.cfg.minmaxData.phase,
							Blob::AMPhaseOverLimitEvt,
							Blob::AMPhaseBelowLimitEvt,
							Blob::AMPhaseInRangeEvt);
			// chequeo factor de potencia
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.pfactor, _amdata.cfg.minmaxData.pfactor,
							Blob::AMPFactorOverLimitEvt,
							Blob::AMPFactorBelowLimitEvt,
							Blob::AMPFactorInRangeEvt);
			// chequeo potencia activa
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.aPow, _amdata.cfg.minmaxData.aPow,
							Blob::AMActPowOverLimitEvt,
							Blob::AMActPowBelowLimitEvt,
							Blob::AMActPowInRangeEvt);
			// chequeo potencia reactiva
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.rPow, _amdata.cfg.minmaxData.rPow,
							Blob::AMReactPowOverLimitEvt,
							Blob::AMReactPowBelowLimitEvt,
							Blob::AMReactPowInRangeEvt);
			// chequeo frecuencia
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.freq, _amdata.cfg.minmaxData.freq,
							Blob::AMFrequencyOverLimitEvt,
							Blob::AMFrequencyBelowLimitEvt,
							Blob::AMFrequencyInRangeEvt);
			// chequeo THD-A
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.thdA, _amdata.cfg.minmaxData.thdA,
							Blob::AMThdAOverLimitEvt,
							Blob::AMThdABelowLimitEvt,
							Blob::AMThdAInRangeEvt);
			// chequeo THD-V
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.thdV, _amdata.cfg.minmaxData.thdV,
							Blob::AMThdVOverLimitEvt,
							Blob::AMThdVBelowLimitEvt,
							Blob::AMThdVInRangeEvt);
		}
		//o si no hay carga activa
		else{
			Blob::AMMinMax_t minmax;
			// chequeo corriente
			minmax = {0, _amdata.analyzers[i].cfg.minmaxData.current.min, _amdata.analyzers[i].cfg.minmaxData.current.thres};
			alarmChecking(	alarm_notif,
							_amdata.analyzers[i].stat.measureValues.current, minmax,
							Blob::AMCurrentOverLimitEvt,
							Blob::AMCurrentBelowLimitEvt,
							Blob::AMCurrentInRangeEvt);
		}
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
		Blob::NotificationData_t<Blob::AMStatData_t> *notif = new Blob::NotificationData_t<Blob::AMStatData_t>(_amdata.stat);
		MBED_ASSERT(notif);
		if(_json_supported){
			cJSON* jstat = JsonParser::getJsonFromNotification(*notif);
			if(jstat){
				char* jmsg = cJSON_Print(jstat);
				cJSON_Delete(jstat);
				MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
				Heap::memFree(jmsg);
				delete(notif);
				Heap::memFree(pub_topic);
				return;
			}
		}

		MQ::MQClient::publish(pub_topic, notif, sizeof(Blob::NotificationData_t<Blob::AMStatData_t>), &_publicationCb);
		delete(notif);
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




