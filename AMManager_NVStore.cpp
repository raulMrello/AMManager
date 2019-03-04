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

// Valores de referencia de Manuel Ariza
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
	_amdata.cfg.verbosity = ESP_LOG_DEBUG;

	// precarga los valores de calibración por defecto
	for(int i=0;i<sizeof(_meter_cal_values)/sizeof(_meter_cal_values[0]);i++){
		_amdata.cfg.calibData.meterRegs[i] = _meter_cal_values[i];
	}
	for(int i=sizeof(_meter_cal_values)/sizeof(_meter_cal_values[0]);i<Blob::AMCalibRegCount;i++){
		_amdata.cfg.calibData.meterRegs[i] = 0;
	}
	for(int i=0;i<sizeof(_meas_cal_values)/sizeof(_meas_cal_values[0]);i++){
		_amdata.cfg.calibData.measRegs[i] = _meas_cal_values[i];
	}
	for(int i=sizeof(_meas_cal_values)/sizeof(_meas_cal_values[0]);i<Blob::AMCalibRegCount;i++){
		_amdata.cfg.calibData.measRegs[i] = 0;
	}

	// establece rangos por defecto
	_amdata.cfg.minmaxData.voltage = {210, 245, 5};
	_amdata.cfg.minmaxData.current = {0.015, 15, 0.005};
	_amdata.cfg.minmaxData.phase = {-185, 185, 5};
	_amdata.cfg.minmaxData.pfactor = {0.8, 1.2, 0.1};
	_amdata.cfg.minmaxData.aPow = {0, 15, 0.01};
	_amdata.cfg.minmaxData.rPow = {0, 15, 0.01};
	_amdata.cfg.minmaxData.msPow = {0, 15, 0.01};
	_amdata.cfg.minmaxData.freq = {49.7, 50.3, 0.1};

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
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Recuperando datos de memoria NV...");
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
	if(!restoreParameter("AMVerbosity", &_amdata.cfg.verbosity, sizeof(esp_log_level_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo verbosity!");
		success = false;
	}

	if(success){
		// chequea el checksum crc32 y después la integridad de los datos
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Datos recuperados. Chequeando integridad...");
		if(Blob::getCRC32(&_amdata.cfg, sizeof(Blob::AMCfgData_t)) != crc){
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
	if(!saveParameter("AMVerbosity", &_amdata.cfg.verbosity, sizeof(esp_log_level_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando verbosity!");
	}
	else{
		esp_log_level_set(_MODULE_, _amdata.cfg.verbosity);
	}
	uint32_t crc = Blob::getCRC32(&_amdata.cfg, sizeof(Blob::AMCfgData_t));
	if(!saveParameter("AMChecksum", &crc, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando Checksum!");
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
	if(keys & Blob::AMKeyCfgVerbosity){
		_amdata.cfg.verbosity = cfg.verbosity;
	}
_updateConfigExit:
	strcpy(err.descr, Blob::errList[err.code]);
}


