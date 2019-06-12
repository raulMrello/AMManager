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

// Valores de referencia de Manuel Ariza M90E26
//------------------------------------------------------------------------------------
const uint16_t AMManager::_meter_cal_values[]  = {
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
const uint16_t AMManager::_meas_cal_values[]  = {
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
bool AMManager::checkIntegrity(){
	#warning TODO
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "~~~~ TODO ~~~~ AMManager::checkIntegrity");
	// Hacer lo que corresponda
	// ...
	if(_amdata.cfg.measPeriod < MeteringManagerCfgMeasPeriodMin)
		return false;
	if(_amdata._numAnalyzers > MeteringManagerCfgMaxNumAnalyzers)
		return false;

	return true;
}


//------------------------------------------------------------------------------------
void AMManager::setDefaultConfig(){
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "~~~~ TODO ~~~~ AMManager::setDefaultConfig");

	// borro la configuración y el estado
	_amdata = {0};

	// lee la versión del driver integrado para formar el uid
	_amdata.uid = UID_METERING_MANAGER;
	for(int i=0;i<MeteringManagerCfgMaxNumAnalyzers;i++){
		_amdata.analyzers[i].uid = UID_METERING_ANALYZER;
	}

	// cargo los datos por defecto: num analizadores, candencia de envío de medidas, verbosidad, eventos...
	_amdata._numAnalyzers 	= _driver->getNumAnalyzers();
	_amdata.cfg.updFlags 	= MeteringManagerCfgUpdNotif;
	_amdata.cfg.measPeriod 	= MeteringManagerCfgMeasPeriodDefault;
	_amdata.cfg.verbosity 	= ESP_LOG_DEBUG;
	_amdata.stat._numAnalyzers = _amdata._numAnalyzers;
	for(int i=0;i<_amdata._numAnalyzers;i++){
		_driver->getAnalyzerSerial(_amdata.analyzers[i].serial, MeteringAnalyzerSerialLength, i);
		_amdata.analyzers[i].cfg.updFlags 			= MeteringManagerCfgUpdNotif;
		_amdata.analyzers[i].cfg.evtFlags 			= MeteringAnalyzerInstantMeasureEvt;
		_amdata.analyzers[i].cfg.minmaxData.voltage = {210.0, 	245.0, 	5.0};
		_amdata.analyzers[i].cfg.minmaxData.current = {0.015, 	15.0, 	0.005};
		_amdata.analyzers[i].cfg.minmaxData.phase 	= {-185.0, 	185.0, 	5.0};
		_amdata.analyzers[i].cfg.minmaxData.pfactor = {0.8, 	1.2, 	0.1};
		_amdata.analyzers[i].cfg.minmaxData.aPow 	= {0.0, 	15.0, 	0.01};
		_amdata.analyzers[i].cfg.minmaxData.rPow 	= {0.0, 	15.0, 	0.01};
		_amdata.analyzers[i].cfg.minmaxData.msPow 	= {0.0, 	15.0, 	0.01};
		_amdata.analyzers[i].cfg.minmaxData.freq 	= {49.7, 	50.3, 	0.1};
		_amdata.analyzers[i].cfg.minmaxData.thdA 	= {0.0, 	1.0, 	0.001};
		_amdata.analyzers[i].cfg.minmaxData.thdV 	= {0.0, 	1.0, 	0.001};
		_amdata.analyzers[i].cfg.minmaxData.active 	= {0.0, 	1.0, 	0.001};
		_amdata.analyzers[i].cfg.minmaxData.reactive= {0.0, 	1.0, 	0.001};
		for(int j=0;j<sizeof(_meter_cal_values)/sizeof(_meter_cal_values[0]);j++){
			_amdata.analyzers[i].cfg.calibData.meterRegs[j] = _meter_cal_values[j];
		}
		for(int j=sizeof(_meter_cal_values)/sizeof(_meter_cal_values[0]);j<MeteringAnalyzerCfgCalibRegCount;j++){
			_amdata.analyzers[i].cfg.calibData.meterRegs[j] = 0;
		}
		for(int j=0;j<sizeof(_meas_cal_values)/sizeof(_meas_cal_values[0]);j++){
			_amdata.analyzers[i].cfg.calibData.measRegs[j] = _meas_cal_values[j];
		}
		for(int j=sizeof(_meas_cal_values)/sizeof(_meas_cal_values[0]);j<MeteringAnalyzerCfgCalibRegCount;j++){
			_amdata.analyzers[i].cfg.calibData.measRegs[j] = 0;
		}
	}

	// guarda la configuración
	saveConfig();
}


//------------------------------------------------------------------------------------
void AMManager::restoreConfig(){
	// inicio el estado por defecto
	_amdata = {0};

	// lee la versión del driver integrado para formar el uid
	_amdata.uid = UID_METERING_MANAGER;
	for(int i=0;i<MeteringManagerCfgMaxNumAnalyzers;i++){
		_amdata.analyzers[i].uid = UID_METERING_ANALYZER;
	}

	// cargo los datos por defecto: num analizadores, candencia de envío de medidas, verbosidad, eventos...
	_amdata._numAnalyzers 	= _driver->getNumAnalyzers();
	_amdata.stat._numAnalyzers = _amdata._numAnalyzers;
	for(int i=0;i<_amdata._numAnalyzers;i++){
		_driver->getAnalyzerSerial(_amdata.analyzers[i].serial, MeteringAnalyzerSerialLength, i);
	}

	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Recuperando datos de memoria NV...");
	bool success = true;
	char nvs_key[16];
	// recupera el objeto metering:manager:cfg
	snprintf(nvs_key, 16, "%s_cfg", _name);
	if(!restoreParameter(nvs_key, &_amdata.cfg, sizeof(metering_manager_cfg), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo %s", _name);
		success = false;
	}

	// recupera la configuración de los analizadores metering:analyzer:cfg
	for(int i=0;i<MeteringManagerCfgMaxNumAnalyzers;i++){
		snprintf(nvs_key, 16, "%s_an%dcfg", _name, i);
		if(!restoreParameter(nvs_key, &_amdata.analyzers[i].cfg, sizeof(metering_analyzer_cfg), NVSInterface::TypeBlob)){
			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo %s", _name);
			success = false;
		}
	}

	uint32_t crc = 0;
	snprintf(nvs_key, 16, "%s_crc", _name);
	if(!restoreParameter(nvs_key, &crc, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo Checksum!");
		success = false;
	}

	if(success){
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Datos recuperados. Chequeando integridad...");

		// chequea el crc
		uint8_t* crc_buf = (char*)Heap::memAlloc(sizeof(metering_manager_cfg) + (MeteringManagerCfgMaxNumAnalyzers*sizeof(metering_analyzer_cfg)));
		MBED_ASSERT(crc_buf);
		memcpy(crc_buf, &_amdata.cfg, sizeof(metering_manager_cfg));
		for(int i=0;i<MeteringManagerCfgMaxNumAnalyzers;i++){
			memcpy(&crc_buf[sizeof(metering_manager_cfg) + (i*sizeof(metering_analyzer_cfg))], &_amdata.analyzers[i].cfg, sizeof(metering_analyzer_cfg));
		}
		uint32_t calc_crc = Blob::getCRC32(crc_buf, sizeof(metering_manager_cfg) + (MeteringManagerCfgMaxNumAnalyzers*sizeof(metering_analyzer_cfg)));
		free(crc_buf);

		if(calc_crc != crc){
			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_CFG. Ha fallado el checksum");
		}
    	else if(!checkIntegrity()){
    		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_CFG. Ha fallado el check de integridad.");
    	}
    	else{
    		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Check de integridad OK!");
    		esp_log_level_set(_MODULE_, _amdata.cfg.verbosity);
    		return;
    	}
	}
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_FS. Error en la recuperación de datos. Establece configuración por defecto");
	setDefaultConfig();
}


//------------------------------------------------------------------------------------
void AMManager::saveConfig(){
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Guardando datos en memoria NV...");
	char nvs_key[16];

	// graba el objeto metering:manager:cfg
	snprintf(nvs_key, 16, "%s_cfg", _name);
	if(!saveParameter(nvs_key, &_amdata.cfg, sizeof(metering_manager_cfg), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando %s", _name);
	}

	// graba la configuración de los analizadores metering:analyzer:cfg
	for(int i=0;i<MeteringManagerCfgMaxNumAnalyzers;i++){
		snprintf(nvs_key, 16, "%s_an%dcfg", _name, i);
		if(!saveParameter(nvs_key, &_amdata.analyzers[i].cfg, sizeof(metering_analyzer_cfg), NVSInterface::TypeBlob)){
			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo %s", _name);
		}
	}

	// genera el crc
	uint8_t* crc_buf = (char*)Heap::memAlloc(sizeof(metering_manager_cfg) + (MeteringManagerCfgMaxNumAnalyzers*sizeof(metering_analyzer_cfg)));
	MBED_ASSERT(crc_buf);
	memcpy(crc_buf, &_amdata.cfg, sizeof(metering_manager_cfg));
	for(int i=0;i<MeteringManagerCfgMaxNumAnalyzers;i++){
		memcpy(&crc_buf[sizeof(metering_manager_cfg) + (i*sizeof(metering_analyzer_cfg))], &_amdata.analyzers[i].cfg, sizeof(metering_analyzer_cfg));
	}
	uint32_t crc = Blob::getCRC32(crc_buf, sizeof(metering_manager_cfg) + (MeteringManagerCfgMaxNumAnalyzers*sizeof(metering_analyzer_cfg)));
	free(crc_buf);

	// graba el crc
	snprintf(nvs_key, 16, "%s_crc", _name);
	if(!saveParameter(nvs_key, &crc, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando Checksum!");
	}

	// aplica el nivel de verbosidad configurado
	esp_log_level_set(_MODULE_, _amdata.cfg.verbosity);
}


//------------------------------------------------------------------------------------
void AMManager::_updateConfig(const metering_manager& data, Blob::ErrorData_t& err){
	err.code = Blob::ErrOK;
	if((data.cfg._keys & (1 << 1))){
		_amdata.cfg.updFlags = data.cfg.updFlags;
	}
	if((data.cfg._keys & (1 << 2))){
		_amdata.cfg.measPeriod = data.cfg.measPeriod;
	}
	if((data.cfg._keys & (1 << 3))){
		_amdata.cfg.verbosity = data.cfg.verbosity;
	}
	// evalúo analizadores
	for(int i=0;i<_amdata._numAnalyzers;i++){
		// evalúo metering:manager:analyzer[]:cfg
		if((data.analyzers[i].cfg._keys & (1 << 1))){
			_amdata.analyzers[i].cfg.updFlags = data.analyzers[i].cfg.updFlags;
		}
		if((data.analyzers[i].cfg._keys & (1 << 2))){
			_amdata.analyzers[i].cfg.evtFlags = data.analyzers[i].cfg.evtFlags;
		}
		// evalúo metering:manager:analyzer[]:minmax:cfg
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 1))){
			_amdata.analyzers[i].cfg.minmaxData.voltage = data.analyzers[i].cfg.minmaxData.voltage;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 2))){
			_amdata.analyzers[i].cfg.minmaxData.current = data.analyzers[i].cfg.minmaxData.current;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 3))){
			_amdata.analyzers[i].cfg.minmaxData.phase = data.analyzers[i].cfg.minmaxData.phase;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 4))){
			_amdata.analyzers[i].cfg.minmaxData.pfactor = data.analyzers[i].cfg.minmaxData.pfactor;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 5))){
			_amdata.analyzers[i].cfg.minmaxData.aPow = data.analyzers[i].cfg.minmaxData.aPow;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 6))){
			_amdata.analyzers[i].cfg.minmaxData.rPow = data.analyzers[i].cfg.minmaxData.rPow;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 7))){
			_amdata.analyzers[i].cfg.minmaxData.msPow = data.analyzers[i].cfg.minmaxData.msPow;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 8))){
			_amdata.analyzers[i].cfg.minmaxData.freq = data.analyzers[i].cfg.minmaxData.freq;
		}
		if(strcmp(_driver->getVersion(), VERS_METERING_EMi10_YTL_NAME) == 0){
			if((data.analyzers[i].cfg.minmaxData._keys & (1 << 9))){
				_amdata.analyzers[i].cfg.minmaxData.thdA = data.analyzers[i].cfg.minmaxData.thdA;
			}
			if((data.analyzers[i].cfg.minmaxData._keys & (1 << 10))){
				_amdata.analyzers[i].cfg.minmaxData.thdV = data.analyzers[i].cfg.minmaxData.thdV;
			}
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 11))){
			_amdata.analyzers[i].cfg.minmaxData.active = data.analyzers[i].cfg.minmaxData.active;
		}
		if((data.analyzers[i].cfg.minmaxData._keys & (1 << 12))){
			_amdata.analyzers[i].cfg.minmaxData.reactive = data.analyzers[i].cfg.minmaxData.reactive;
		}

		// evalúo metering:manager:analyzer[]:calib:cfg
		if(strcmp(_driver->getVersion(), VERS_METERING_M90E26_NAME) == 0){
			if((data.analyzers[i].cfg.calibData._keys & (1 << 1))){
				for(int j=0; j<MeteringAnalyzerCfgCalibRegCount; j++)
					_amdata.analyzers[i].cfg.calibData.meterRegs[j] = data.analyzers[i].cfg.calibData.meterRegs[j];
			}
			if((data.analyzers[i].cfg.calibData._keys & (1 << 2))){
				for(int j=0; j<MeteringAnalyzerCfgCalibRegCount; j++)
					_amdata.analyzers[i].cfg.calibData.measRegs[j] = data.analyzers[i].cfg.calibData.measRegs[j];
			}
		}
	}

	strcpy(err.descr, Blob::errList[err.code]);
}


