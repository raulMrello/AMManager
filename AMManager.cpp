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
    _driver_list.push_back(driver);

	// Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &AMManager::publicationCb);

    // crea el timer para el worker de medida
	_meas_tmr = NULL;

}


//------------------------------------------------------------------------------------
AMManager::AMManager(std::list<AMDriver*> driver_list, FSManager* fs, bool defdbg, const char* name) : ActiveModule(name, osPriorityNormal, 4096, fs, defdbg), _name(name) {

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

	// copia la lista de drivers
    _driver_list = driver_list;

	// Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &AMManager::publicationCb);

    // crea el timer para el worker de medida
	_meas_tmr = NULL;

}

//------------------------------------------------------------------------------------
void AMManager::startMeasureWork() {
	if(_meas_tmr != NULL){
		_meas_tmr->stop();
		delete(_meas_tmr);
		_meas_tmr = NULL;
	}

	_instant_meas_counter = _amdata.cfg.measPeriod / (DefaultMeasurePeriod/1000);
	// crea el timer para el worker de medida
	_meas_tmr = new RtosTimer(callback(this, &AMManager::eventMeasureWorkCb), osTimerPeriodic, "AMMeasWork");
	// realiza una medida con una cadencia dada
	_meas_tmr->start(DefaultMeasurePeriod);
}


//------------------------------------------------------------------------------------
void AMManager::stopMeasureWork() {
	if(_meas_tmr != NULL){
		delete(_meas_tmr);
		_meas_tmr = NULL;
	}
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
	bool alarm_notif[MeteringManagerCfgMaxNumAnalyzers];

	// lee todos los parámetros eléctricos de cada analizador
	int i = 0;
	for(auto drv = _driver_list.begin(); drv != _driver_list.end(); ++drv){
		AMDriver* am_driver = (*drv);
		int analyz = am_driver->getNumAnalyzers();
		for(int a = 0; a < analyz; a++){
			// en caso de tener más analizadores que los registrados, marca error y sale de los bucles
			if(i >= _amdata._numAnalyzers){
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error en numero de analizadores medidos. max=%d", _amdata._numAnalyzers);
				goto __exit_measure_loop;
			}
			AMDriver::ElectricParams eparam;
			uint32_t keys;
			if(am_driver->getElectricParams(eparam, keys, i) == AMDriver::Success){
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
			// chequeo cada una de las alarmas:
			alarm_notif[i] = false;

			// en primer lugar si hay carga activa
			if(_amdata.stat.loadPercent[i] > 0){
				// chequeo tensión
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.voltage, _amdata.analyzers[i].cfg.minmaxData.voltage,
								MeteringAnalyzerVoltageOverLimitEvt,
								MeteringAnalyzerVoltageBelowLimitEvt,
								0);
				// chequeo corriente
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.current, _amdata.analyzers[i].cfg.minmaxData.current,
								MeteringAnalyzerCurrentOverLimitEvt,
								MeteringAnalyzerCurrentBelowLimitEvt,
								0);
				// chequeo phase
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.phase, _amdata.analyzers[i].cfg.minmaxData.phase,
								MeteringAnalyzerPhaseOverLimitEvt,
								MeteringAnalyzerPhaseBelowLimitEvt,
								0);
				// chequeo factor de potencia
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.pfactor, _amdata.analyzers[i].cfg.minmaxData.pfactor,
								MeteringAnalyzerPFactorOverLimitEvt,
								MeteringAnalyzerPFactorBelowLimitEvt,
								0);
				// chequeo potencia activa
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.aPow, _amdata.analyzers[i].cfg.minmaxData.aPow,
								MeteringAnalyzerActPowOverLimitEvt,
								MeteringAnalyzerActPowBelowLimitEvt,
								0);
				// chequeo potencia reactiva
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.rPow, _amdata.analyzers[i].cfg.minmaxData.rPow,
								MeteringAnalyzerReactPowOverLimitEvt,
								MeteringAnalyzerReactPowBelowLimitEvt,
								0);
				// chequeo frecuencia
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.freq, _amdata.analyzers[i].cfg.minmaxData.freq,
								MeteringAnalyzerFrequencyOverLimitEvt,
								MeteringAnalyzerFrequencyBelowLimitEvt,
								0);
				// chequeo THD-A
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.thdA, _amdata.analyzers[i].cfg.minmaxData.thdA,
								MeteringAnalyzerThdAOverLimitEvt,
								MeteringAnalyzerThdABelowLimitEvt,
								0);
				// chequeo THD-V
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.thdV, _amdata.analyzers[i].cfg.minmaxData.thdV,
								MeteringAnalyzerThdVOverLimitEvt,
								MeteringAnalyzerThdVBelowLimitEvt,
								0);
				// chequeo energía activa
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.energyValues.active, _amdata.analyzers[i].cfg.minmaxData.active,
								MeteringAnalyzerActEnergyOverLimitEvt,
								0,
								0);
				// chequeo energía activa
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.energyValues.reactive, _amdata.analyzers[i].cfg.minmaxData.reactive,
								MeteringAnalyzerReactEnergyOverLimitEvt,
								0,
								0);
			}
			//o si no hay carga activa
			else{
				// chequeo corriente
				common_range_minmaxthres_double minmax = {0, _amdata.analyzers[i].cfg.minmaxData.current.min, _amdata.analyzers[i].cfg.minmaxData.current.thres};
				alarmChecking(	alarm_notif[i],
								_amdata.analyzers[i].stat.flags,
								_amdata.analyzers[i].cfg.evtFlags,
								_amdata.analyzers[i].stat.measureValues.current, minmax,
								MeteringAnalyzerCurrentOverLimitEvt,
								MeteringAnalyzerCurrentBelowLimitEvt,
								0);
			}
			// incremento el identificador del analizador analizado
			i++;
		}
	}
__exit_measure_loop:

	// cada N medidas, envía un evento de medida para no saturar las comunicaciones
	if(--_instant_meas_counter <= 0){
		_instant_meas_counter = _amdata.cfg.measPeriod / (DefaultMeasurePeriod/1000);
		for(int i=0; i<_amdata._numAnalyzers; i++){
			alarm_notif[i] = true;
			_amdata.analyzers[i].stat.flags |= MeteringAnalyzerInstantMeasureEvt;
		}
	}
	else{
		for(int i=0; i<_amdata._numAnalyzers; i++){
			_amdata.analyzers[i].stat.flags &= ~MeteringAnalyzerInstantMeasureEvt;
		}
	}

	// notifica alarmas
	for(int i=0; i<_amdata._numAnalyzers; i++){
		if(alarm_notif[i] && enable_notif){
			// envía mensaje con los flags que se han activado y que están operativos
			DEBUG_TRACE_D(_EXPR_, _MODULE_, "Notificando evento %x", _amdata.analyzers[i].stat.flags);
			char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/value/%s", _pub_topic_base);
			Blob::NotificationData_t<metering_manager> *notif = new Blob::NotificationData_t<metering_manager>(_amdata);
			MBED_ASSERT(notif);
			if(_json_supported){
				cJSON* jstat = JsonParser::getJsonFromNotification(*notif, ObjSelectState);
				MBED_ASSERT(jstat);
				MQ::MQClient::publish(pub_topic, &jstat, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jstat);
			}
			else {
				MQ::MQClient::publish(pub_topic, notif, sizeof(Blob::NotificationData_t<metering_manager>), &_publicationCb);
			}
			delete(notif);
			Heap::memFree(pub_topic);
		}
	}
}


//------------------------------------------------------------------------------------
void AMManager::alarmChecking(	bool& alarm_notif,
								uint32_t& flags,
								uint32_t flagmask,
								double measure,
								common_range_minmaxthres_double data_range,
								uint32_t flag_over_limit,
								uint32_t flag_below_limit,
								uint32_t flag_in_range){

	if(measure > data_range.max && (flags & flag_over_limit) == 0){
		flags |= flag_over_limit;
		// si el flag está activado, notifica evento
		if(flagmask & flag_over_limit){
			alarm_notif = true;
		}
	}
	else if(measure < data_range.min && (flags & flag_below_limit) == 0){
		flags |= flag_below_limit;
		// si el flag está activado, notifica evento
		if(flagmask & flag_below_limit){
			alarm_notif = true;
		}
	}
	else if((measure >= (data_range.min + data_range.thres) && measure <= (data_range.max - data_range.thres)) && (flags & (flag_over_limit|flag_below_limit)) != 0){
		flags &= ~(flag_over_limit|flag_below_limit);
		//_amdata.stat.flags |= flag_in_range;

		// si el flag está activado o no éste no se utiliza, notifica evento al volver al rango
		if((flagmask & flag_in_range) || (flag_in_range==0)){
			alarm_notif = true;
		}
	}
}




