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
    _meas_started = false;
    _drv_errors = 0;
	// Carga callbacks est�ticas de publicaci�n/suscripci�n
    _publicationCb = callback(this, &AMManager::publicationCb);

}

void AMManager::addDriver(AMDriver *driver){
	DriverObj* dobj = new DriverObj();
	MBED_ASSERT(dobj);
	dobj->drv = driver;
	dobj->cycle_ms = 0;
	dobj->measures = NULL;
	dobj->readings = NULL;
	_driver_list.push_back(dobj);
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
    _acc_errors = 0;
    _meas_started = false;
    _drv_errors = 0;

	// Carga callbacks est�ticas de publicaci�n/suscripci�n
    _publicationCb = callback(this, &AMManager::publicationCb);
}

void AMManager::startMeasureWorkDriverShelly(char* name){
	for(auto i= _driver_list.begin(); i!=_driver_list.end(); ++i){
		DriverObj* dobj = (*i);
		AMDriver* drv = (dobj->drv);
		if(strcmp(drv->getVersion(), name))
		{
			if(strcmp(drv->getVersion(), VERS_METERING_AM_SHELLY0)==0){
				// establece el ciclo de lectura
				dobj->cycle_ms = VERS_METERING_AM_SHELLY0_NAME;
				// crea los objetos de medida de cada analizador
				// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
				// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
				// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
				AMDriver::AutoMeasureObj* amo_block = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_MeasureBlock),AMDriver::AllAnalyzers);
				MBED_ASSERT(amo_block);
				dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
				MBED_ASSERT(dobj->measures);

				// forma la lista de medida con los objetos anteriores
				dobj->measures->push_back(amo_block);
				// idem con los objetos de lectura
				AMDriver::AutoMeasureReading* sh_m = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(sh_m);
				sh_m->analyzer=0;
				AMDriver::AutoMeasureReading* sh_r = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(sh_r);
				sh_r->analyzer=1;
				dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
				MBED_ASSERT(dobj->readings);
				dobj->readings->push_back(sh_m);
				dobj->readings->push_back(sh_r);

				// solicita el inicio de medidas peri�dicas
				if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
					// si falla, destruye los objetos creados
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
	}
}

//------------------------------------------------------------------------------------
void AMManager::startMeasureWork(bool discard_ext_anlz) {
	if(_meas_started)
		return;
	_discard_ext_anlz = discard_ext_anlz;
	_acc_errors = 0;

	// este arranque aplica para todos los drivers instalados
	// a parte de planificar las lecturas, es posible que dependiendo del tipo de driver, haya que
	// configurar los par�metros que se desean leer.
	for(auto i= _driver_list.begin(); i!=_driver_list.end(); ++i){
		DriverObj* dobj = (*i);
		AMDriver* drv = (dobj->drv);
		// si es un driver AMUniConnectors planifica una medida peri�dica cada segundo de los par�metros
		// en bloque
		if(strcmp(drv->getVersion(), VERS_METERING_AM_UNI_CONNECTORS_NAME)==0){

			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_UNI_CONNECTORS_MEASCYCLE;
			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
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

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
			}
		}else if(strcmp(drv->getVersion(), VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME)==0){
			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
			// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
			AMDriver::AutoMeasureObj* amo_block = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_MeasureBlock),AMDriver::AllAnalyzers);
			MBED_ASSERT(amo_block);
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);

			// forma la lista de medida con los objetos anteriores
			dobj->measures->push_back(amo_block);

			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_COMBIPLUS_CONNECTORS_MEASCYCLE;
			AMDriver::AutoMeasureReading* amr_r1 = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_r1);
			amr_r1->analyzer=0;
			AMDriver::AutoMeasureReading* amr_s1 = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_s1);
			amr_s1->analyzer=1;
			AMDriver::AutoMeasureReading* amr_t1 = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_t1);
			amr_t1->analyzer=2;
			AMDriver::AutoMeasureReading* amr_r2 = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_r2);
			amr_r2->analyzer=3;
			AMDriver::AutoMeasureReading* amr_s2 = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_r2);
			amr_s2->analyzer=4;
			AMDriver::AutoMeasureReading* amr_t2 = new AMDriver::AutoMeasureReading();
			MBED_ASSERT(amr_r2);
			amr_t2->analyzer=5;
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);
			dobj->readings->push_back(amr_r1);
			dobj->readings->push_back(amr_s1);
			dobj->readings->push_back(amr_t1);
			dobj->readings->push_back(amr_r2);
			dobj->readings->push_back(amr_s2);
			dobj->readings->push_back(amr_t2);

			// solicita el inicio de medidas peri�dicas
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

		// si es un driver TMC100 planifica una medida peri�dica cada segundo de los par�metros
		// en bloque
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_MBUS012_NAME)==0){

			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_MBUS012_MEASCYCLE;

			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
			// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);

			// forma la lista de medida con los objetos anteriores
			for(uint8_t i=0; i<VERS_METERING_AM_MBUS012_ANALYZERS; i++){
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_Current), i);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);

				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver TMC100");
			}
		}
		// si es un driver TMC100 planifica una medida peri�dica cada segundo de los par�metros
		// en bloque
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_MBUS0_NAME)==0){
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_MBUS0_MEASCYCLE;

			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
			// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);

			// forma la lista de medida con los objetos anteriores
			for(uint8_t i=0; i<VERS_METERING_AM_MBUS0_ANALYZERS; i++){
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_Current), i);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);

				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver TMC100");
			}
		}
		// si es un driver TMC100 planifica una medida peri�dica cada segundo de los par�metros
		// en bloque
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_MBUS03_NAME)==0){
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_MBUS03_MEASCYCLE;

			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
			// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);

			// forma la lista de medida con los objetos anteriores
			for(uint8_t i=0; i<VERS_METERING_AM_MBUS03_ANALYZERS; i++){
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_Current), i);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);

				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver TMC100");
			}
		}

		// si es un driver Driver_Ctx0643 planifica una medida peri�dica cada segundo de los par�metros
		// en bloque
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_CTX1_NAME)==0){
			if(discard_ext_anlz){
				dobj->cycle_ms = 0;
				continue;
			}
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_CTX1_MEASCYCLE;

			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);

			AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_Current|AMDriver::ElecKey_Voltage|AMDriver::ElecKey_ActivePow|AMDriver::ElecKey_ReactivePow|AMDriver::ElecKey_ActiveEnergy|AMDriver::ElecKey_ReactiveEnergy|AMDriver::ElecKey_PowFactor), 0);
			MBED_ASSERT(amo);
			dobj->measures->push_back(amo);

			// forma la lista de medida con los objetos anteriores
			for(uint8_t i=0; i<VERS_METERING_AM_CTX1_ANALYZERS; i++){

				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver Driver_Ctx0643");
			}
		}
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_CTX3_NAME)==0){
			if(discard_ext_anlz){
				dobj->cycle_ms = 0;
				continue;
			}
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_CTX3_MEASCYCLE;

			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);

			if(dobj->drv->getModel() == VERS_METERING_AM_CTX3_MODEL_DTS353){
				for(uint8_t i=0; i<VERS_METERING_AM_CTX3_ANALYZERS; i++){
					// la fase R es la única que lee el agregado de energías activas y reactivas trifásicas
					if(i==0){
						AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_Current|AMDriver::ElecKey_Voltage|AMDriver::ElecKey_ActivePow|AMDriver::ElecKey_ReactivePow|AMDriver::ElecKey_ActiveEnergy|AMDriver::ElecKey_ReactiveEnergy|AMDriver::ElecKey_PowFactor), i);
						MBED_ASSERT(amo);
						dobj->measures->push_back(amo);
					}
					else{
						AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_Current|AMDriver::ElecKey_Voltage|AMDriver::ElecKey_ActivePow|AMDriver::ElecKey_ReactivePow|AMDriver::ElecKey_PowFactor), i);
						MBED_ASSERT(amo);
						dobj->measures->push_back(amo);
					}
				}
			}
			else{
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_Current|AMDriver::ElecKey_Voltage|AMDriver::ElecKey_ActivePow|AMDriver::ElecKey_PowFactor), AMDriver::AllAnalyzers);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);
			}

			// forma la lista de medida con los objetos anteriores
			for(uint8_t i=0; i<VERS_METERING_AM_CTX3_ANALYZERS; i++){

				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver Driver_Ctx0643");
			}
		}
		// si es un driver Driver_Ctx0643 planifica una medida peri�dica cada segundo de los par�metros
		// en bloque
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_MID1_NAME)==0){
			if(discard_ext_anlz){
				dobj->cycle_ms = 0;
				continue;
			}
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_MID1_MEASCYCLE;

			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);


			// forma la lista de medida con los objetos anteriores
			for(uint8_t i=0; i<VERS_METERING_AM_MID1_ANALYZERS; i++){
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_ActiveEnergy|AMDriver::ElecKey_ReactiveEnergy), i);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);
				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver Driver_Mid1x1");
			}
		}
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_MID3_NAME)==0){
			if(discard_ext_anlz){
				dobj->cycle_ms = 0;
				continue;
			}
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_MID3_MEASCYCLE;

			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);

			for(uint8_t i=0; i<VERS_METERING_AM_MID3_ANALYZERS; i++){
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_ActiveEnergy|AMDriver::ElecKey_ReactiveEnergy), i);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);
				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver Driver_Mid3x1");
			}
		}
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_MID1x2_NAME)==0){
			if(discard_ext_anlz){
				dobj->cycle_ms = 0;
				continue;
			}
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_MID1x2_MEASCYCLE;
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);


			// forma la lista de medida con los objetos anteriores
			for(uint8_t i=0; i<VERS_METERING_AM_MID1x2_ANALYZERS; i++){
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_ActiveEnergy|AMDriver::ElecKey_ReactiveEnergy), i);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);

				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver Driver_Mid1x2");
			}
		}		
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_MID3x2_NAME)==0){
			if(discard_ext_anlz){
				dobj->cycle_ms = 0;
				continue;
			}
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_MID3x2_MEASCYCLE;
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);

			for(uint8_t i=0; i<VERS_METERING_AM_MID3x2_ANALYZERS; i++){
				AMDriver::AutoMeasureObj* amo = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_ActiveEnergy|AMDriver::ElecKey_ReactiveEnergy), i);
				MBED_ASSERT(amo);
				dobj->measures->push_back(amo);

				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer=i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver Driver_Mid3x2");
			}
		}
		else if (strcmp(drv->getVersion(), VERS_METERING_AM_PVINV_NAME) == 0) {
			// Inversor solar
			if (discard_ext_anlz) {
				dobj->cycle_ms = 0;
				continue;
			}

			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_PVINV_MEASCYCLE;
			// crea los objetos de medida de cada analizador (medidas a realizar y lecturas)
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);
			// Solicitud por bloques
			AMDriver::AutoMeasureObj* amo_block = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_MeasureBlock), AMDriver::AllAnalyzers);
			MBED_ASSERT(amo_block);
			dobj->measures->push_back(amo_block);
			for (uint8_t i = 0; i < VERS_METERING_AM_PVINV_ANALYZERS; i++) {
				// 0-paneles, 1-baterías, 2-Salida_Inversor_fase_R, 3-Salida_Inversor_fase_S, 4-Salida_Inversor_fase_T
				// 5-Contador_externo_fase_R, 6-Contador_externo_fase_S, 7-Contador_externo_fase_T, 
				AMDriver::AutoMeasureReading* amr = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(amr);
				amr->analyzer = i;
				dobj->readings->push_back(amr);
			}

			// solicita el inicio de medidas periódicas
			if (dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures) != 0) {
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
				DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error iniciando medidas automaticas en driver PVInverter");
			}
		}
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_SHELLY0_NAME)==0){
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_SHELLY0_MEASCYCLE;
			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
			// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
			AMDriver::AutoMeasureObj* amo_block = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_MeasureBlock),AMDriver::AllAnalyzers);
			MBED_ASSERT(amo_block);
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			// forma la lista de medida con los objetos anteriores
			dobj->measures->push_back(amo_block);
			
			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);
			for(uint8_t i = 0; i < VERS_METERING_AM_SHELLY0_ANALYZERS; i++){
				// idem con los objetos de lectura
				AMDriver::AutoMeasureReading* sh = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(sh);
				sh->analyzer=i;
				dobj->readings->push_back(sh);
			}
			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
				cpp_utils::list_delete_items(*dobj->readings);
				delete(dobj->readings);
				dobj->readings = NULL;
				cpp_utils::list_delete_items(*dobj->measures);
				delete(dobj->measures);
				dobj->measures = NULL;
				dobj->cycle_ms = 0;
			}
		}
		else if(strcmp(drv->getVersion(), VERS_METERING_AM_SHELLY1_NAME)==0){
			// establece el ciclo de lectura
			dobj->cycle_ms = VERS_METERING_AM_SHELLY1_MEASCYCLE;
			// crea los objetos de medida de cada analizador
			// NOTA: los ElecKeys no son necesarios ya que la medida en bloque lee todo, de todas formas lo especifico
			// para que se entienda qu� es lo que quiero leer. Lo que s� es importante es especificar que se quieren
			// leer todos los analyzadores AMDriver::AllAnalyzers (esto es lo que desencadena la medida en bloque)
			AMDriver::AutoMeasureObj* amo_block = new AMDriver::AutoMeasureObj((uint32_t)(AMDriver::ElecKey_MeasureBlock),AMDriver::AllAnalyzers);
			MBED_ASSERT(amo_block);
			dobj->measures = new std::list<AMDriver::AutoMeasureObj*>();
			MBED_ASSERT(dobj->measures);
			// forma la lista de medida con los objetos anteriores
			dobj->measures->push_back(amo_block);

			dobj->readings = new std::list<AMDriver::AutoMeasureReading*>();
			MBED_ASSERT(dobj->readings);
			for(uint8_t i = 0; i < VERS_METERING_AM_SHELLY1_ANALYZERS; i++){
				// idem con los objetos de lectura
				AMDriver::AutoMeasureReading* sh = new AMDriver::AutoMeasureReading();
				MBED_ASSERT(sh);
				sh->analyzer=i;
				dobj->readings->push_back(sh);
			}
			// solicita el inicio de medidas peri�dicas
			if(dobj->drv->startPeriodicMeasurement(dobj->cycle_ms, *dobj->measures)!=0){
				// si falla, destruye los objetos creados
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

	

	// arranca el timer de lectura
	_instant_meas_counter = _amdata.cfg.measPeriod;
	// crea el timer para el worker de medida
	_meas_tmr.attach_us(callback(this, &AMManager::eventMeasureWorkCb), 1000*DefaultMeasurePeriod);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando medidas automaticas cada %d ms", DefaultMeasurePeriod);
	_meas_started = true;
}


//------------------------------------------------------------------------------------
void AMManager::stopMeasureWork() {
	if(!_meas_started)
		return;

	DEBUG_TRACE_W(_EXPR_, _MODULE_, "Finalizando medidas automaticas");
	_meas_tmr.detach();
	// este arranque aplica para todos los drivers instalados
	// a parte de planificar las lecturas, es posible que dependiendo del tipo de driver, haya que
	// configurar los par�metros que se desean leer.
	for(auto i= _driver_list.begin(); i!=_driver_list.end(); ++i){
		DriverObj* dobj = (*i);
		AMDriver* drv = (dobj->drv);

		// si es un driver AMUniConnectors planifica una medida peri�dica cada segundo de los par�metros
		// de tensi�n y corriente de sus 4 analizadores
#ifndef COMBI_PLUS
		if(strcmp(drv->getVersion(), VERS_METERING_AM_UNI_CONNECTORS_NAME)==0){
#else
		if(strcmp(drv->getVersion(), VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME)==0){
#endif
			cpp_utils::list_delete_items(*dobj->readings);
			delete(dobj->readings);
			dobj->readings = NULL;
			cpp_utils::list_delete_items(*dobj->measures);
			delete(dobj->measures);
			dobj->measures = NULL;
			dobj->cycle_ms = 0;
		}
	}
	_meas_started = false;
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
	bool reading_hw_error = false;

	// lee todos los par�metros el�ctricos de cada analizador
	int i = 0;
	int acc_analyzers = 0;
	bool any_update = false;
	int base_analyzer = 0;
	for(auto d = _driver_list.begin(); d != _driver_list.end(); ++d){
		DriverObj* dobj = (*d);
		AMDriver* am_driver = dobj->drv;

		base_analyzer += acc_analyzers;
		acc_analyzers = am_driver->getNumAnalyzers();

		// si el driver tiene habilitado las medidas autom�ticas, las ejecuta
		if(dobj->cycle_ms > 0 && dobj->measures && dobj->readings){
			int32_t gar_res = dobj->drv->getAnalyzerReadings(*dobj->readings);
			if(gar_res==0){
				_acc_errors = 0;
				// eval�a las nuevas medidas
				for(auto r = dobj->readings->begin(); r != dobj->readings->end(); ++r){
					AMDriver::AutoMeasureReading* amr = (*r);
					DEBUG_TRACE_D(_EXPR_, _MODULE_, "base_analyzer=%d, acc_analyzers=%d, amr->analyzer=%d", base_analyzer, acc_analyzers, amr->analyzer);
					uint32_t keys = amr->params.measureId;
					alarm_notif[(base_analyzer + amr->analyzer)] = false;
					_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags = 0;
					// visualiza los par�metros le�dos
					if(keys != 0){
						if(keys & AMDriver::ElecKey_Voltage){
#ifdef COMBI_PLUS
							if(amr->params.voltage > (double)Blob::AMMaxAllowedVoltage && strcmp(dobj->drv->getVersion(), VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME)==0)
#else
							if((amr->params.voltage > (double)Blob::AMMaxAllowedVoltage && strcmp(dobj->drv->getVersion(), VERS_METERING_AM_UNI_CONNECTORS_NAME)==0) ||
							   (amr->params.voltage > (double)Blob::AMMaxAllowedVoltage && strcmp(dobj->drv->getVersion(), VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME)==0))
#endif
							{
								reading_hw_error = true;
								DEBUG_TRACE_E(_EXPR_, _MODULE_, "Analizador=[%d], Voltage=%.01fV ERROR (descartado)", (base_analyzer + amr->analyzer),amr->params.voltage);
							}
							else{
								_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.voltage = amr->params.voltage;
								_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerVoltage;
								DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], Voltage=%.01fV", (base_analyzer + amr->analyzer),_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.voltage);
							}
						}
						if(keys & AMDriver::ElecKey_Current){
#ifdef COMBI_PLUS
							if(amr->params.current > (double)Blob::AMMaxAllowedCurrent && strcmp(dobj->drv->getVersion(), VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME)==0)
#else
							if((amr->params.current > (double)Blob::AMMaxAllowedCurrent && strcmp(dobj->drv->getVersion(), VERS_METERING_AM_UNI_CONNECTORS_NAME)==0) ||
							   (amr->params.current > (double)Blob::AMMaxAllowedCurrent && strcmp(dobj->drv->getVersion(), VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME)==0))
#endif
							{
								reading_hw_error = true;
								DEBUG_TRACE_E(_EXPR_, _MODULE_, "Analizador=[%d], Current=%.03fA ERROR (descartado)", (base_analyzer + amr->analyzer),amr->params.current);
							}
							else{
								_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.current = amr->params.current;
								_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerCurrent;
								DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], Current=%.03fA", (base_analyzer + amr->analyzer),_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.current);
							}
						}
						if(keys & AMDriver::ElecKey_ActivePow){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.aPow = amr->params.aPow;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerActivePower;
							DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], aPow=%.02fW", (base_analyzer + amr->analyzer),_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.aPow);
						}
						if(keys & AMDriver::ElecKey_ReactivePow){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.rPow = amr->params.rPow;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerReactivePower;
							DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], rPow=%.02fVA", (base_analyzer + amr->analyzer),_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.rPow);
						}
						if(keys & AMDriver::ElecKey_ApparentPow){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.msPow = amr->params.mPow;
						}
						if(keys & AMDriver::ElecKey_PowFactor){
							double pfactor = abs(amr->params.pFactor);
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.pfactor = pfactor;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerPowerFactor;
							if(pfactor < METERING_PFACTOR_MIN_ALLOWED){
								DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], pfactor=%.02f corregido a %.02f", (base_analyzer + amr->analyzer),pfactor, METERING_PFACTOR_MIN_ALLOWED);
								_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.pfactor = METERING_PFACTOR_MIN_ALLOWED;
							}
							else{
								DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], pfactor=%.02f", (base_analyzer + amr->analyzer),_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.pfactor);
							}
						}
						if(keys & AMDriver::ElecKey_THDAmpere){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.thdA = amr->params.thdAmp;
						}
						if(keys & AMDriver::ElecKey_THDVoltage){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.thdV = amr->params.thdVolt;
						}
						if(keys & AMDriver::ElecKey_Frequency){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.freq = amr->params.freq;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerFrequency;
							DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], Freq=%.2fHz", (base_analyzer + amr->analyzer), _amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.freq);
						}
						if(keys & AMDriver::ElecKey_ActiveEnergy){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.energyValues.active = amr->params.aEnergy;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerEnergyActive;
							DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], Active=%.02fWh", (base_analyzer + amr->analyzer), _amdata.analyzers[(base_analyzer + amr->analyzer)].stat.energyValues.active);
						}
						if(keys & AMDriver::ElecKey_ReactiveEnergy){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.energyValues.reactive = amr->params.rEnergy;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerEnergyReactive;
							DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], Reactive=%.02fWh", (base_analyzer + amr->analyzer), _amdata.analyzers[(base_analyzer + amr->analyzer)].stat.energyValues.reactive);
						}
						if(keys & AMDriver::ElecKey_Status){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.status = amr->params.status;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerStatus;
							DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], Status=%.02f", (base_analyzer + amr->analyzer), _amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.status);
						}
						if(keys & AMDriver::Eleckey_Temperature){
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.temp = amr->params.temp;
							_amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags |= MeteringAnalyzerTemperature;
							DEBUG_TRACE_D(_EXPR_, _MODULE_, "Analizador=[%d], Temp=%.2fºC", (base_analyzer + amr->analyzer), _amdata.analyzers[(base_analyzer + amr->analyzer)].stat.measureValues.temp);
						}
					}
					any_update = (alarm_notif[(base_analyzer + amr->analyzer)])? true : any_update;
					DEBUG_TRACE_D(_EXPR_, _MODULE_, "stat.flags[%d] = %x", (base_analyzer + amr->analyzer), _amdata.analyzers[(base_analyzer + amr->analyzer)].stat.flags);
				}
			}
		}
	}
//__exit_measure_loop:

	// cada N medidas, env�a un evento de medida para no saturar las comunicaciones
	if(--_instant_meas_counter <= 0){
		any_update = true;
		_instant_meas_counter = _amdata.cfg.measPeriod;
		for(int i=0; i<_amdata._numAnalyzers; i++){
			if(_amdata.analyzers[i].stat.flags & MeteringAnalyzerElectricParam){
				alarm_notif[i] = true;
				_amdata.analyzers[i].stat.flags |= MeteringAnalyzerInstantMeasureEvt;
				// corrige el sentido de la corriente si es positiva y potencia negativa
				if(_amdata.analyzers[i].stat.measureValues.aPow < 0 && _amdata.analyzers[i].stat.measureValues.current > 0){
					_amdata.analyzers[i].stat.measureValues.current = -_amdata.analyzers[i].stat.measureValues.current;
				}
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
				// corrige el sentido de la corriente si es positiva y potencia negativa
				if(_amdata.analyzers[i].stat.measureValues.aPow < 0 && _amdata.analyzers[i].stat.measureValues.current > 0){
					_amdata.analyzers[i].stat.measureValues.current = -_amdata.analyzers[i].stat.measureValues.current;
				}
			}
		}
	}
	else{
		for(int i=0; i<_amdata._numAnalyzers; i++){
			_amdata.analyzers[i].stat.flags &= ~MeteringAnalyzerInstantMeasureEvt;
		}
	}

	// notifica un �nico mensaje que engloba a todos los analizadores
	if(any_update && enable_notif && !reading_hw_error){
		// env�a mensaje con los flags que se han activado y que est�n operativos
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
		// si el flag est� activado, notifica evento
		if(flagmask & flag_over_limit){
			alarm_notif = true;
		}
	}
	else if(measure < data_range.min && (flags & flag_below_limit) == 0){
		flags |= flag_below_limit;
		// si el flag est� activado, notifica evento
		if(flagmask & flag_below_limit){
			alarm_notif = true;
		}
	}
	else if((measure >= (data_range.min + data_range.thres) && measure <= (data_range.max - data_range.thres)) && (flags & (flag_over_limit|flag_below_limit)) != 0){
		flags &= ~(flag_over_limit|flag_below_limit);
		//_amdata.stat.flags |= flag_in_range;

		// si el flag est� activado o no �ste no se utiliza, notifica evento al volver al rango
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

	// responde con los datos solicitados y con los errores (si hubiera) de la decodificaci�n de la solicitud
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
void AMManager::_responseWithAnalyzers(uint32_t idTrans, Blob::ErrorData_t& err){
	// prepara el topic al que responder
	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
	MBED_ASSERT(pub_topic);
	sprintf(pub_topic, "stat/analyzers/%s", _pub_topic_base);

	// responde con los datos solicitados y con los errores (si hubiera) de la decodificaci�n de la solicitud
	Blob::Response_t<metering_manager>* resp = new Blob::Response_t<metering_manager>(idTrans, err, _amdata);
	MBED_ASSERT(resp);
	if(_json_supported){
		cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectStateSub);
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

	// responde con los datos solicitados y con los errores (si hubiera) de la decodificaci�n de la solicitud
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
	// Notifica el cambio de estado general del componente
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Enviando nueva medida en stat/value/energy");
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


//------------------------------------------------------------------------------------
void AMManager::eventMeasureWorkCb(){
    // crea el mensaje para publicar en la m�quina de estados
     State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
     MBED_ASSERT(op);

    op->sig = TimedMeasureEvt;
	// apunta a los datos
	op->msg = NULL;

	// postea en la cola de la m�quina de estados
	if(putMessage(op) != osOK){
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al insertar evento de medida");
		if(op->msg){
			Heap::memFree(op->msg);
		}
		Heap::memFree(op);
	}
 }



