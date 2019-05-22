/*
 * test_metering_objects.cpp
 *
 *	Test unitario para los objetos metering del módulo AMManager
 */


//------------------------------------------------------------------------------------
//-- TEST HEADERS --------------------------------------------------------------------
//------------------------------------------------------------------------------------

#include "unity.h"
#include "JsonParserBlob.h"

//------------------------------------------------------------------------------------
//-- REQUIRED HEADERS & COMPONENTS FOR TESTING ---------------------------------------
//------------------------------------------------------------------------------------

#include "AppConfig.h"

/** variables requeridas para realizar el test */
static void executePrerequisites();
static bool s_test_done = false;


//------------------------------------------------------------------------------------
//-- SPECIFIC COMPONENTS FOR TESTING -------------------------------------------------
//------------------------------------------------------------------------------------

static const char* _MODULE_ = "[TEST_metering].";
#define _EXPR_	(true)

static metering_manager obj;
static metering_manager obj_back;
//------------------------------------------------------------------------------------
//-- TEST CASES ----------------------------------------------------------------------
//------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/** Se verifica la codificación JSON del objeto completo
 *
 */
TEST_CASE("Test Obj->Json->Obj from Whole Obj........................", "[metering_objs]") {

	// ejecuta requisitos previos
	executePrerequisites();

	obj_back = obj;
	TEST_ASSERT_EQUAL(memcmp(&obj, &obj_back, sizeof(metering_manager)), 0);

	// Conversión Obj -> JSON
	cJSON* json = JsonParser::getJsonFromObj(obj, ObjSelectAll);
	TEST_ASSERT_NOT_NULL(json);
	char* json_msg = cJSON_PrintUnformatted(json);
	TEST_ASSERT_NOT_NULL(json_msg);
	cJSON_Delete(json);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", json_msg);

	// Conversión JSON -> Obj
	JsonParser::getObjFromJson(obj_back, json_msg);
	json = JsonParser::getJsonFromObj(obj_back, ObjSelectAll);
	TEST_ASSERT_NOT_NULL(json);
	char* json_msg_2 = cJSON_PrintUnformatted(json);
	TEST_ASSERT_NOT_NULL(json_msg_2);
	cJSON_Delete(json);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Obj_back = %s", json_msg_2);
	int result = strcmp(json_msg, json_msg_2);
	free(json_msg_2);
	free(json_msg);

	// Evaluación
	TEST_ASSERT_EQUAL(0, result);
}


//---------------------------------------------------------------------------
/** Se verifica la codificación JSON del estado del objeto
 *
 */
TEST_CASE("Test Obj->Json->Obj from State Obj........................", "[metering_objs]") {

	// ejecuta requisitos previos
	executePrerequisites();

	obj_back = obj;
	TEST_ASSERT_EQUAL(memcmp(&obj, &obj_back, sizeof(metering_manager)), 0);

	// Conversión Obj -> JSON
	cJSON* json = JsonParser::getJsonFromObj(obj, ObjSelectState);
	TEST_ASSERT_NOT_NULL(json);
	char* json_msg = cJSON_PrintUnformatted(json);
	TEST_ASSERT_NOT_NULL(json_msg);
	cJSON_Delete(json);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", json_msg);

	// Conversión JSON -> Obj
	JsonParser::getObjFromJson(obj_back, json_msg);
	json = JsonParser::getJsonFromObj(obj_back, ObjSelectState);
	TEST_ASSERT_NOT_NULL(json);
	char* json_msg_2 = cJSON_PrintUnformatted(json);
	TEST_ASSERT_NOT_NULL(json_msg_2);
	cJSON_Delete(json);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Obj_back = %s", json_msg_2);
	int result = strcmp(json_msg, json_msg_2);
	free(json_msg_2);
	free(json_msg);

	// Evaluación
	TEST_ASSERT_EQUAL(0, result);
}


//---------------------------------------------------------------------------
/** Se verifica la codificación JSON de la configuración del objeto
 *
 */
TEST_CASE("Test Obj->Json->Obj from Config Obj.......................", "[metering_objs]") {

	// ejecuta requisitos previos
	executePrerequisites();

	obj_back = obj;
	TEST_ASSERT_EQUAL(memcmp(&obj, &obj_back, sizeof(metering_manager)), 0);

	// Conversión Obj -> JSON
	cJSON* json = JsonParser::getJsonFromObj(obj, ObjSelectCfg);
	TEST_ASSERT_NOT_NULL(json);
	char* json_msg = cJSON_PrintUnformatted(json);
	TEST_ASSERT_NOT_NULL(json_msg);
	cJSON_Delete(json);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", json_msg);

	// Conversión JSON -> Obj
	JsonParser::getObjFromJson(obj_back, json_msg);
	json = JsonParser::getJsonFromObj(obj_back, ObjSelectCfg);
	TEST_ASSERT_NOT_NULL(json);
	char* json_msg_2 = cJSON_PrintUnformatted(json);
	TEST_ASSERT_NOT_NULL(json_msg_2);
	cJSON_Delete(json);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Obj_back = %s", json_msg_2);
	int result = strcmp(json_msg, json_msg_2);
	free(json_msg_2);
	free(json_msg);

	// Evaluación
	TEST_ASSERT_EQUAL(0, result);
}


//---------------------------------------------------------------------------
/** Se configura el objeto para utilizar un driver EMi10 YTL
 *
 */
TEST_CASE("Config Emi10 YTL with 1 analyzers for re-test.............", "[metering_objs]") {

	// ejecuta requisitos previos
	executePrerequisites();

	// borro la configuración y el estado
	obj = {0};

	obj.uid 		= UID_METERING_MANAGER(VERS_METERING_EMi10_YTL);
	obj.cfg.uid 	= UID_METERING_MANAGER_CFG(VERS_METERING_EMi10_YTL);
	obj.stat.uid	= UID_METERING_MANAGER_STAT(VERS_METERING_EMi10_YTL);
	for(int i=0;i<1;i++){
		obj.analyzers[i].uid 					= UID_METERING_ANALYZER(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].cfg.uid 				= UID_METERING_ANALYZER_CFG(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].cfg.minmaxData.uid 	= UID_METERING_ANALYZER_CFG_MINMAX(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].cfg.calibData.uid 		= UID_METERING_ANALYZER_CFG_CALIB(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].stat.uid				= UID_METERING_ANALYZER_STAT(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].stat.energyValues.uid	= UID_METERING_ANALYZER_STAT_TOTALS(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].stat.measureValues.uid	= UID_METERING_ANALYZER_STAT_MEASURE(VERS_METERING_EMi10_YTL);
	}


	// cargo los datos por defecto: num analizadores, candencia de envío de medidas, verbosidad, eventos...
	obj._numAnalyzers 	= 1;
	obj.cfg.updFlags 	= MeteringManagerCfgUpdNotif;
	obj.cfg.measPeriod 	= MeteringManagerCfgMeasPeriodDefault;
	obj.cfg.verbosity 	= ESP_LOG_DEBUG;
	obj.stat._numAnalyzers = obj._numAnalyzers;
	for(int i=0;i<obj._numAnalyzers;i++){
		sprintf(obj.analyzers[i].serial, "EMi10 YTL AN%d",i);
		obj.analyzers[i].cfg.updFlags 			= MeteringManagerCfgUpdNotif;
		obj.analyzers[i].cfg.evtFlags 			= MeteringAnalyzerInstantMeasureEvt;
		obj.analyzers[i].cfg.minmaxData.voltage = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 210.0, 	245.0, 	5.0};
		obj.analyzers[i].cfg.minmaxData.current = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.015, 	15.0, 	0.005};
		obj.analyzers[i].cfg.minmaxData.phase 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), -185.0, 	185.0, 	5.0};
		obj.analyzers[i].cfg.minmaxData.pfactor = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.8, 	1.2, 	0.1};
		obj.analyzers[i].cfg.minmaxData.aPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.rPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.msPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.freq 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 49.7, 	50.3, 	0.1};
		obj.analyzers[i].cfg.minmaxData.thdA 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	1.0, 	0.001};
		obj.analyzers[i].cfg.minmaxData.thdV 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	1.0, 	0.001};
		for(int i=0;i<sizeof(obj.analyzers[i].cfg.calibData.meterRegs)/sizeof(obj.analyzers[i].cfg.calibData.meterRegs[0]);i++){
			obj.analyzers[i].cfg.calibData.meterRegs[i] = 0;
		}
		for(int i=sizeof(obj.analyzers[i].cfg.calibData.meterRegs)/sizeof(obj.analyzers[i].cfg.calibData.meterRegs[0]);i<MeteringAnalyzerCfgCalibRegCount;i++){
			obj.analyzers[i].cfg.calibData.meterRegs[i] = 0;
		}
		for(int i=0;i<sizeof(obj.analyzers[i].cfg.calibData.measRegs)/sizeof(obj.analyzers[i].cfg.calibData.measRegs[0]);i++){
			obj.analyzers[i].cfg.calibData.measRegs[i] = 0;
		}
		for(int i=sizeof(obj.analyzers[i].cfg.calibData.measRegs)/sizeof(obj.analyzers[i].cfg.calibData.measRegs[0]);i<MeteringAnalyzerCfgCalibRegCount;i++){
			obj.analyzers[i].cfg.calibData.measRegs[i] = 0;
		}
	}
}


//---------------------------------------------------------------------------
/** Se configura el objeto para utilizar un driver EMi10 YTL con 3 analizadores
 *
 */
TEST_CASE("Config Emi10 YTL with 3 analyzers for re-test.............", "[metering_objs]") {

	// ejecuta requisitos previos
	executePrerequisites();

	// borro la configuración y el estado
	obj = {0};

	obj.uid 		= UID_METERING_MANAGER(VERS_METERING_EMi10_YTL);
	obj.cfg.uid 	= UID_METERING_MANAGER_CFG(VERS_METERING_EMi10_YTL);
	obj.stat.uid	= UID_METERING_MANAGER_STAT(VERS_METERING_EMi10_YTL);
	for(int i=0;i<3;i++){
		obj.analyzers[i].uid 					= UID_METERING_ANALYZER(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].cfg.uid 				= UID_METERING_ANALYZER_CFG(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].cfg.minmaxData.uid 	= UID_METERING_ANALYZER_CFG_MINMAX(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].cfg.calibData.uid 		= UID_METERING_ANALYZER_CFG_CALIB(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].stat.uid				= UID_METERING_ANALYZER_STAT(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].stat.energyValues.uid	= UID_METERING_ANALYZER_STAT_TOTALS(VERS_METERING_EMi10_YTL);
		obj.analyzers[i].stat.measureValues.uid	= UID_METERING_ANALYZER_STAT_MEASURE(VERS_METERING_EMi10_YTL);
	}


	// cargo los datos por defecto: num analizadores, candencia de envío de medidas, verbosidad, eventos...
	obj._numAnalyzers 	= 3;
	obj.cfg.updFlags 	= MeteringManagerCfgUpdNotif;
	obj.cfg.measPeriod 	= MeteringManagerCfgMeasPeriodDefault;
	obj.cfg.verbosity 	= ESP_LOG_DEBUG;
	obj.stat._numAnalyzers = obj._numAnalyzers;
	for(int i=0;i<obj._numAnalyzers;i++){
		sprintf(obj.analyzers[i].serial, "EMi10 YTL AN%d",i);
		obj.analyzers[i].cfg.updFlags 			= MeteringManagerCfgUpdNotif;
		obj.analyzers[i].cfg.evtFlags 			= MeteringAnalyzerInstantMeasureEvt;
		obj.analyzers[i].cfg.minmaxData.voltage = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 210.0, 	245.0, 	5.0};
		obj.analyzers[i].cfg.minmaxData.current = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.015, 	15.0, 	0.005};
		obj.analyzers[i].cfg.minmaxData.phase 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), -185.0, 	185.0, 	5.0};
		obj.analyzers[i].cfg.minmaxData.pfactor = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.8, 	1.2, 	0.1};
		obj.analyzers[i].cfg.minmaxData.aPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.rPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.msPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.freq 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 49.7, 	50.3, 	0.1};
		obj.analyzers[i].cfg.minmaxData.thdA 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	1.0, 	0.001};
		obj.analyzers[i].cfg.minmaxData.thdV 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	1.0, 	0.001};
		for(int i=0;i<sizeof(obj.analyzers[i].cfg.calibData.meterRegs)/sizeof(obj.analyzers[i].cfg.calibData.meterRegs[0]);i++){
			obj.analyzers[i].cfg.calibData.meterRegs[i] = 0;
		}
		for(int i=sizeof(obj.analyzers[i].cfg.calibData.meterRegs)/sizeof(obj.analyzers[i].cfg.calibData.meterRegs[0]);i<MeteringAnalyzerCfgCalibRegCount;i++){
			obj.analyzers[i].cfg.calibData.meterRegs[i] = 0;
		}
		for(int i=0;i<sizeof(obj.analyzers[i].cfg.calibData.measRegs)/sizeof(obj.analyzers[i].cfg.calibData.measRegs[0]);i++){
			obj.analyzers[i].cfg.calibData.measRegs[i] = 0;
		}
		for(int i=sizeof(obj.analyzers[i].cfg.calibData.measRegs)/sizeof(obj.analyzers[i].cfg.calibData.measRegs[0]);i<MeteringAnalyzerCfgCalibRegCount;i++){
			obj.analyzers[i].cfg.calibData.measRegs[i] = 0;
		}
	}
}


//---------------------------------------------------------------------------
/** Se configura el objeto para utilizar un driver M90E26
 *
 */
TEST_CASE("Config M90E26 with 1 analyzers for re-test................", "[metering_objs]") {

	// ejecuta requisitos previos
	executePrerequisites();

	// borro la configuración y el estado
	obj = {0};

	obj.uid 		= UID_METERING_MANAGER(VERS_METERING_M90E26);
	obj.cfg.uid 	= UID_METERING_MANAGER_CFG(VERS_METERING_M90E26);
	obj.stat.uid	= UID_METERING_MANAGER_STAT(VERS_METERING_M90E26);
	for(int i=0;i<1;i++){
		obj.analyzers[i].uid 					= UID_METERING_ANALYZER(VERS_METERING_M90E26);
		obj.analyzers[i].cfg.uid 				= UID_METERING_ANALYZER_CFG(VERS_METERING_M90E26);
		obj.analyzers[i].cfg.minmaxData.uid 	= UID_METERING_ANALYZER_CFG_MINMAX(VERS_METERING_M90E26);
		obj.analyzers[i].cfg.calibData.uid 		= UID_METERING_ANALYZER_CFG_CALIB(VERS_METERING_M90E26);
		obj.analyzers[i].stat.uid				= UID_METERING_ANALYZER_STAT(VERS_METERING_M90E26);
		obj.analyzers[i].stat.energyValues.uid	= UID_METERING_ANALYZER_STAT_TOTALS(VERS_METERING_M90E26);
		obj.analyzers[i].stat.measureValues.uid	= UID_METERING_ANALYZER_STAT_MEASURE(VERS_METERING_M90E26);
	}

	// cargo los datos por defecto: num analizadores, candencia de envío de medidas, verbosidad, eventos...
	obj._numAnalyzers 	= 1;
	obj.cfg.updFlags 	= MeteringManagerCfgUpdNotif;
	obj.cfg.measPeriod 	= MeteringManagerCfgMeasPeriodDefault;
	obj.cfg.verbosity 	= ESP_LOG_DEBUG;
	obj.stat._numAnalyzers = obj._numAnalyzers;
	for(int i=0;i<obj._numAnalyzers;i++){
		sprintf(obj.analyzers[i].serial, "M90E26 AN%d",i);
		obj.analyzers[i].cfg.updFlags 			= MeteringManagerCfgUpdNotif;
		obj.analyzers[i].cfg.evtFlags 			= MeteringAnalyzerInstantMeasureEvt;
		obj.analyzers[i].cfg.minmaxData.voltage = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 210.0, 	245.0, 	5.0};
		obj.analyzers[i].cfg.minmaxData.current = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.015, 	15.0, 	0.005};
		obj.analyzers[i].cfg.minmaxData.phase 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), -185.0, 	185.0, 	5.0};
		obj.analyzers[i].cfg.minmaxData.pfactor = {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.8, 	1.2, 	0.1};
		obj.analyzers[i].cfg.minmaxData.aPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.rPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.msPow 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	15.0, 	0.01};
		obj.analyzers[i].cfg.minmaxData.freq 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 49.7, 	50.3, 	0.1};
		obj.analyzers[i].cfg.minmaxData.thdA 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	1.0, 	0.001};
		obj.analyzers[i].cfg.minmaxData.thdV 	= {UID_COMMON_RANGE_MINMAXTHRES_DOUBLE(0), 0.0, 	1.0, 	0.001};
		for(int i=0;i<sizeof(obj.analyzers[i].cfg.calibData.meterRegs)/sizeof(obj.analyzers[i].cfg.calibData.meterRegs[0]);i++){
			obj.analyzers[i].cfg.calibData.meterRegs[i] = 0;
		}
		for(int i=sizeof(obj.analyzers[i].cfg.calibData.meterRegs)/sizeof(obj.analyzers[i].cfg.calibData.meterRegs[0]);i<MeteringAnalyzerCfgCalibRegCount;i++){
			obj.analyzers[i].cfg.calibData.meterRegs[i] = 0;
		}
		for(int i=0;i<sizeof(obj.analyzers[i].cfg.calibData.measRegs)/sizeof(obj.analyzers[i].cfg.calibData.measRegs[0]);i++){
			obj.analyzers[i].cfg.calibData.measRegs[i] = 0;
		}
		for(int i=sizeof(obj.analyzers[i].cfg.calibData.measRegs)/sizeof(obj.analyzers[i].cfg.calibData.measRegs[0]);i<MeteringAnalyzerCfgCalibRegCount;i++){
			obj.analyzers[i].cfg.calibData.measRegs[i] = 0;
		}
	}
}


//------------------------------------------------------------------------------------
//-- PREREQUISITES -------------------------------------------------------------------
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
static void executePrerequisites(){
	/** Prerequisites execution control flag */
	static bool s_executed_prerequisites = false;
	if(!s_executed_prerequisites){
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Ready for test!");
		obj = {0};
		// marca flag de prerequisitos completado
		s_executed_prerequisites = true;
	}
}


