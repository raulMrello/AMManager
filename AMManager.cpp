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
    DriverObj* dobj = new DriverObj();
    MBED_ASSERT(dobj);
    dobj->drv = driver;
    dobj->cycle_ms = 0;
    dobj->measures = NULL;
    dobj->readings = NULL;
    _driver_list.push_back(dobj);
    _acc_errors = 0;

	// Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &AMManager::publicationCb);

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
    for(auto i=driver_list.begin(); i!=driver_list.end();++i){
    	AMDriver* driver = (*i);
        DriverObj* dobj = new DriverObj();
        MBED_ASSERT(dobj);
        dobj->drv = driver;
        dobj->cycle_ms = 0;
        dobj->measures = NULL;
        dobj->readings = NULL;
        _driver_list.push_back(dobj);
    }

	// Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &AMManager::publicationCb);
}

//------------------------------------------------------------------------------------
void AMManager::startMeasureWork() {
	_acc_errors = 0;

	// este arranque aplica para todos los drivers instalados
	// a parte de planificar las lecturas, es posible que dependiendo del tipo de driver, haya que
	// configurar los parámetros que se desean leer.
	for(auto i= _driver_list.begin(); i!=_driver_list.end(); ++i){
		DriverObj* dobj = (*i);
		AMDriver* drv = (dobj->drv);

		// si es un driver AMUniConnectors planifica una medida periódica cada segundo de los parámetros
		// en bloque
		if(strcmp(drv->getVersion(), VERS_METERING_AM_UNI_CONNECTORS_NAME)==0){
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_UNI_CONNECTORS_MEASCYCLE;

			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qué es lo que quiero leer. Lo que sí es importante es especificar que se quieren
			// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
			AMDriver::AutoMeasureObj* amo_block = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_MeasureBlock),AMDriver::AllAnalyzers);
			MBED_ASSERT(amo_block);
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);

			// forma la lista de medida con los objetos anteriores
			dobj->measures->push_back(amo_block);

			// idem con los objetos de lectura
			AMDriver::AutoMeasureReading* amr_m = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_m);
			amr_m->analyzer=0;
			AMDriver::AutoMeasureReading* amr_r = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_r);
			amr_r->analyzer=1;
			AMDriver::AutoMeasureReading* amr_s = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_s);
			amr_s->analyzer=2;
			AMDriver::AutoMeasureReading* amr_t = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_t);
			amr_t->analyzer=3;
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);
			dobj->readings->push_back(amr_m);
			dobj->readings->push_back(amr_r);
			dobj->readings->push_back(amr_s);
			dobj->readings->push_back(amr_t);

			// solicita el inicio de medidas periódicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver AMUniConnectors");
			}
		}
	}

	// arranca el timer de lectura
	_instant_meas_counter = _amdata.cfg.measPeriod / (DefaultMeasurePeriod/1000);
	// crea el timer para el worker de medida
	_meas_tmr.attach_us(callback(this, &AMManager::eventMeasureWorkCb), 1000*DefaultMeasurePeriod);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando medidas automaticas cada %d ms", DefaultMeasurePeriod);
}


//------------------------------------------------------------------------------------
void AMManager::stopMeasureWork() {
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "Finalizando medidas automaticas");
	_meas_tmr.detach();
	// este arranque aplica para todos los drivers instalados
	// a parte de planificar las lecturas, es posible que dependiendo del tipo de driver, haya que
	// configurar los parámetros que se desean leer.
	for(auto i= _driver_list.begin(); i!=_driver_list.end(); ++i){
		DriverObj* dobj = (*i);
		AMDriver* drv = (dobj->drv);

		// si es un driver AMUniConnectors planifica una medida periódica cada segundo de los parámetros
		// de tensión y corriente de sus 4 analizadores
		if(strcmp(drv->getVersion(), VERS_METERING_AM_UNI_CONNECTORS_NAME)==0){
			cpp_utils::list_delete_items(*dobj->readings);
			delete(dobj->readings);
			dobj->readings = NULL;
			cpp_utils::list_delete_items(*dobj->measures);
			delete(dobj->measures);
			dobj->measures = NULL;
			dobj->cycle_ms = 0;
		}
	}
}


//------------------------------------------------------------------------------------
void AMManager::setLoggingLevel(esp_log_level_t level){
	esp_log_level_set(_MODULE_, level);
}



//------------------------------------------------------------------------------------
//-- PROTECTED METHODS IMPLEMENTATION ------------------------------------------------
//------------------------------------------------------------------------------------



//------------------------------------------------------------------------------------
void AMManager::publicationCb(const char* topic, int32_t result){

}


//------------------------------------------------------------------------------------
void AMManager::_measure(bool enable_notif) {
	Heap::printHeap();
	double value;
	uint32_t multiplier = 1000;
	bool alarm_notif[MeteringManagerCfgMaxNumAnalyzers];

	// lee todos los parámetros eléctricos de cada analizador
	int i = 0;
	bool any_update = false;
	for(auto d = _driver_list.begin(); d != _driver_list.end(); ++d){
		DriverObj* dobj = (*d);
		AMDriver* am_driver = dobj->drv;

		// si el driver tiene habilitado las medidas automáticas, las ejecuta
		if(dobj->cycle_ms > 0 && dobj->measures && dobj->readings){
			// realiza la lectura
			DEBUG_TRACE_D(_EXPR_, _MODULE_, "Leyendo lecturas");

			SafetyOp* op = new SafetyOp(__FUNCTION__, 5000, callback(&esp_restart));
			MBED_ASSERT(op);
			int32_t gar_res = dobj->drv->getAnalyzerReadings(*dobj->readings);
			delete(op);
			if(gar_res==0){
				_acc_errors = 0;
				// evalúa las nuevas medidas
				for(auto r = dobj->readings->begin(); r != dobj->readings->end(); ++r){
					AMDriver::AutoMeasureReading* amr = (*r);
					uint32_t keys = amr->params.measureId;
					alarm_notif[amr->analyzer] = false;

					// visualiza los parámetros leídos
					if(keys != 0){
						if(keys & AMDriver::ElecKey_Voltage){
							_amdata.analyzers[amr->analyzer].stat.measureValues.voltage = amr->params.voltage;
							_amdata.analyzers[amr->analyzer].stat.flags |= MeteringAnalyzerVoltage;
						}
						if(keys & AMDriver::ElecKey_Current){
							_amdata.analyzers[amr->analyzer].stat.measureValues.current = amr->params.current;
							_amdata.analyzers[amr->analyzer].stat.flags |= MeteringAnalyzerCurrent;
						}
						if(keys & AMDriver::ElecKey_ActivePow){
							_amdata.analyzers[amr->analyzer].stat.measureValues.aPow = amr->params.aPow;
							_amdata.analyzers[amr->analyzer].stat.flags |= MeteringAnalyzerActivePower;
						}
						if(keys & AMDriver::ElecKey_ReactivePow){
							_amdata.analyzers[amr->analyzer].stat.measureValues.rPow = amr->params.rPow;
							_amdata.analyzers[amr->analyzer].stat.flags |= MeteringAnalyzerReactivePower;
						}
						if(keys & AMDriver::ElecKey_ApparentPow){
							_amdata.analyzers[amr->analyzer].stat.measureValues.msPow = amr->params.mPow;
						}
						if(keys & AMDriver::ElecKey_PowFactor){
							_amdata.analyzers[amr->analyzer].stat.measureValues.pfactor = amr->params.pFactor;
							_amdata.analyzers[amr->analyzer].stat.flags |= MeteringAnalyzerPowerFactor;
						}
						if(keys & AMDriver::ElecKey_THDAmpere){
							_amdata.analyzers[amr->analyzer].stat.measureValues.thdA = amr->params.thdAmp;
						}
						if(keys & AMDriver::ElecKey_THDVoltage){
							_amdata.analyzers[amr->analyzer].stat.measureValues.thdV = amr->params.thdVolt;
						}
						if(keys & AMDriver::ElecKey_Frequency){
							_amdata.analyzers[amr->analyzer].stat.measureValues.freq = amr->params.freq;
							_amdata.analyzers[amr->analyzer].stat.flags |= MeteringAnalyzerFrequency;
						}
						if(keys & AMDriver::ElecKey_ActiveEnergy){
							_amdata.analyzers[amr->analyzer].stat.energyValues.active = amr->params.aEnergy;
						}
						if(keys & AMDriver::ElecKey_ReactiveEnergy){
							_amdata.analyzers[amr->analyzer].stat.energyValues.reactive = amr->params.rEnergy;
						}
						// chequeo cada una de las alarmas:
						// en primer lugar si hay carga activa
						if(_amdata.stat.loadPercent[amr->analyzer] > 0){
							// chequeo tensión
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.voltage, _amdata.analyzers[amr->analyzer].cfg.minmaxData.voltage,
											MeteringAnalyzerVoltageOverLimitEvt,
											MeteringAnalyzerVoltageBelowLimitEvt,
											0);
							// chequeo corriente
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.current, _amdata.analyzers[amr->analyzer].cfg.minmaxData.current,
											MeteringAnalyzerCurrentOverLimitEvt,
											MeteringAnalyzerCurrentBelowLimitEvt,
											0);
							// chequeo phase
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.phase, _amdata.analyzers[amr->analyzer].cfg.minmaxData.phase,
											MeteringAnalyzerPhaseOverLimitEvt,
											MeteringAnalyzerPhaseBelowLimitEvt,
											0);
							// chequeo factor de potencia
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.pfactor, _amdata.analyzers[amr->analyzer].cfg.minmaxData.pfactor,
											MeteringAnalyzerPFactorOverLimitEvt,
											MeteringAnalyzerPFactorBelowLimitEvt,
											0);
							// chequeo potencia activa
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.aPow, _amdata.analyzers[amr->analyzer].cfg.minmaxData.aPow,
											MeteringAnalyzerActPowOverLimitEvt,
											MeteringAnalyzerActPowBelowLimitEvt,
											0);
							// chequeo potencia reactiva
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.rPow, _amdata.analyzers[amr->analyzer].cfg.minmaxData.rPow,
											MeteringAnalyzerReactPowOverLimitEvt,
											MeteringAnalyzerReactPowBelowLimitEvt,
											0);
							// chequeo frecuencia
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.freq, _amdata.analyzers[amr->analyzer].cfg.minmaxData.freq,
											MeteringAnalyzerFrequencyOverLimitEvt,
											MeteringAnalyzerFrequencyBelowLimitEvt,
											0);
							// chequeo THD-A
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.thdA, _amdata.analyzers[amr->analyzer].cfg.minmaxData.thdA,
											MeteringAnalyzerThdAOverLimitEvt,
											MeteringAnalyzerThdABelowLimitEvt,
											0);
							// chequeo THD-V
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.thdV, _amdata.analyzers[amr->analyzer].cfg.minmaxData.thdV,
											MeteringAnalyzerThdVOverLimitEvt,
											MeteringAnalyzerThdVBelowLimitEvt,
											0);
							// chequeo energía activa
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.energyValues.active, _amdata.analyzers[amr->analyzer].cfg.minmaxData.active,
											MeteringAnalyzerActEnergyOverLimitEvt,
											0,
											0);
							// chequeo energía activa
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.energyValues.reactive, _amdata.analyzers[amr->analyzer].cfg.minmaxData.reactive,
											MeteringAnalyzerReactEnergyOverLimitEvt,
											0,
											0);
						}
						//o si no hay carga activa
						else{
							// chequeo corriente
							common_range_minmaxthres_double minmax = {0, _amdata.analyzers[amr->analyzer].cfg.minmaxData.current.min, _amdata.analyzers[amr->analyzer].cfg.minmaxData.current.thres};
							alarmChecking(	alarm_notif[amr->analyzer],
											_amdata.analyzers[amr->analyzer].stat.flags,
											_amdata.analyzers[amr->analyzer].cfg.evtFlags,
											_amdata.analyzers[amr->analyzer].stat.measureValues.current, minmax,
											MeteringAnalyzerCurrentOverLimitEvt,
											MeteringAnalyzerCurrentBelowLimitEvt,
											0);
						}
					}
					any_update = (alarm_notif[amr->analyzer])? true : any_update;
				}
			}
		}
		// en caso contrario ejecuta la lectura de todos los parámetros manualmente analizador por analizador
		else{
			// en otro caso, las lee de forma manual analizador por analizador
			int analyz = am_driver->getNumAnalyzers();

			for(int a = 0; a < analyz; a++){
				alarm_notif[i] = false;
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
						_amdata.analyzers[i].stat.flags |= MeteringAnalyzerVoltage;
					}
					if(keys & AMDriver::ElecKey_Current){
						_amdata.analyzers[i].stat.measureValues.current = eparam.current;
						_amdata.analyzers[i].stat.flags |= MeteringAnalyzerCurrent;
					}
					if(keys & AMDriver::ElecKey_ActivePow){
						_amdata.analyzers[i].stat.measureValues.aPow = eparam.aPow;
						_amdata.analyzers[i].stat.flags |= MeteringAnalyzerActivePower;
					}
					if(keys & AMDriver::ElecKey_ReactivePow){
						_amdata.analyzers[i].stat.measureValues.rPow = eparam.rPow;
						_amdata.analyzers[i].stat.flags |= MeteringAnalyzerReactivePower;
					}
					if(keys & AMDriver::ElecKey_ApparentPow){
						_amdata.analyzers[i].stat.measureValues.msPow = eparam.mPow;
					}
					if(keys & AMDriver::ElecKey_PowFactor){
						_amdata.analyzers[i].stat.measureValues.pfactor = eparam.pFactor;
						_amdata.analyzers[i].stat.flags |= MeteringAnalyzerPowerFactor;
					}
					if(keys & AMDriver::ElecKey_THDAmpere){
						_amdata.analyzers[i].stat.measureValues.thdA = eparam.thdAmp;
					}
					if(keys & AMDriver::ElecKey_THDVoltage){
						_amdata.analyzers[i].stat.measureValues.thdV = eparam.thdVolt;
					}
					if(keys & AMDriver::ElecKey_Frequency){
						_amdata.analyzers[i].stat.measureValues.freq = eparam.freq;
						_amdata.analyzers[i].stat.flags |= MeteringAnalyzerFrequency;
					}
					if(keys & AMDriver::ElecKey_ActiveEnergy){
						_amdata.analyzers[i].stat.energyValues.active = eparam.aEnergy;
					}
					if(keys & AMDriver::ElecKey_ReactiveEnergy){
						_amdata.analyzers[i].stat.energyValues.reactive = eparam.rEnergy;
					}
				}
				// chequeo cada una de las alarmas:
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
				any_update = (alarm_notif[i])? true : any_update;
				// incremento el identificador del analizador analizado
				i++;
			}
		}
	}
__exit_measure_loop:

	// cada N medidas, envía un evento de medida para no saturar las comunicaciones
	if(--_instant_meas_counter <= 0){
		any_update = true;
		_instant_meas_counter = _amdata.cfg.measPeriod / (DefaultMeasurePeriod/1000);
		for(int i=0; i<_amdata._numAnalyzers; i++){
			if(_amdata.analyzers[i].stat.flags & MeteringAnalyzerElectricParam){
				alarm_notif[i] = true;
				_amdata.analyzers[i].stat.flags |= MeteringAnalyzerInstantMeasureEvt;
			}
		}
	}
	else if(_forced_notification){
		_forced_notification = false;
		any_update = true;
		for(int i=0; i<_amdata._numAnalyzers; i++){
			if(_amdata.analyzers[i].stat.flags & MeteringAnalyzerElectricParam){
				alarm_notif[i] = true;
				_amdata.analyzers[i].stat.flags |= MeteringAnalyzerInstantMeasureEvt;
			}
		}
	}
	else{
		for(int i=0; i<_amdata._numAnalyzers; i++){
			_amdata.analyzers[i].stat.flags &= ~MeteringAnalyzerInstantMeasureEvt;
		}
	}

	// notifica un único mensaje que engloba a todos los analizadores
	if(any_update && enable_notif){
		// envía mensaje con los flags que se han activado y que están operativos
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Notificando evento");
		_notifyState();
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


//------------------------------------------------------------------------------------
void AMManager::_responseWithState(uint32_t idTrans, Blob::ErrorData_t& err){
	// prepara el topic al que responder
	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
	MBED_ASSERT(pub_topic);
	sprintf(pub_topic, "stat/value/%s", _pub_topic_base);

	// responde con los datos solicitados y con los errores (si hubiera) de la decodificación de la solicitud
	Blob::Response_t<metering_manager>* resp = new Blob::Response_t<metering_manager>(idTrans, err, _amdata);
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
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Enviada respuesta con estado solicitado");
}


//------------------------------------------------------------------------------------
void AMManager::_responseWithConfig(uint32_t idTrans, Blob::ErrorData_t& err){
   	// prepara el topic al que responder
	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
	MBED_ASSERT(pub_topic);
	sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

	// responde con los datos solicitados y con los errores (si hubiera) de la decodificación de la solicitud
	Blob::Response_t<metering_manager>* resp = new Blob::Response_t<metering_manager>(idTrans, err, _amdata);
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
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Enviada respuesta con configuracion solicitada");
}


//------------------------------------------------------------------------------------
void AMManager::_notifyState(){
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
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Enviada notificacion de cambio de estado");
}


//------------------------------------------------------------------------------------
void AMManager::eventMeasureWorkCb(){
    // crea el mensaje para publicar en la máquina de estados
     State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
     MBED_ASSERT(op);

     op->sig = TimedMeasureEvt;
	// apunta a los datos
	op->msg = NULL;

	// postea en la cola de la máquina de estados
	if(putMessage(op) != osOK){
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al insertar evento de medida");
		if(op->msg){
			Heap::memFree(op->msg);
		}
		Heap::memFree(op);
	}
 }



