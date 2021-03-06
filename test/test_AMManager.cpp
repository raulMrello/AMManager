/*
 * test_AstCalendar.cpp
 *
 *	Test unitario para el m�dulo AMManager
 */


//------------------------------------------------------------------------------------
//-- TEST HEADERS --------------------------------------------------------------------
//------------------------------------------------------------------------------------

#include "unity.h"
#include "AMManager.h"
#if defined(VERS_METERING_VIRTUALAM)
#include "VirtualAMDriver.h"
static VirtualAMDriver* vd = NULL;

#endif
#if defined(VERS_METERING_EMi10_YTL)
#include "YTLAMDriver.h"
static YTLBus* ytlbus = NULL;
static YTLAMDriver* ytl = NULL;

#endif
#if defined(VERS_METERING_M90E26)
#include "M90E26.h"
static M90E26* m90e26 = NULL;
#endif

//------------------------------------------------------------------------------------
//-- REQUIRED HEADERS & COMPONENTS FOR TESTING ---------------------------------------
//------------------------------------------------------------------------------------

#include "AppConfig.h"
#include "FSManager.h"
#include "cJSON.h"

/** variables requeridas para realizar el test */
static FSManager* fs=NULL;
static MQ::PublishCallback s_published_cb;
static void subscriptionCb(const char* topic, void* msg, uint16_t msg_len);
static void publishedCb(const char* topic, int32_t result);
static void executePrerequisites();
static bool s_test_done = false;


//------------------------------------------------------------------------------------
//-- SPECIFIC COMPONENTS FOR TESTING -------------------------------------------------
//------------------------------------------------------------------------------------

static AMManager* energy=NULL;
static const char* _MODULE_ = "[TEST_AMMan]....";
#define _EXPR_	(true)





//------------------------------------------------------------------------------------
//-- TEST CASES ----------------------------------------------------------------------
//------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * @brief Se verifica la creaci�n del objeto y la suscripci�n a topics
 * MQLib
 */
#if defined(VERS_METERING_VIRTUALAM)
TEST_CASE("Create AMManager with VirtualDriver.......................", "[AMManager]") {

	// ejecuta requisitos previos
	executePrerequisites();

	// crea el objeto
	TEST_ASSERT_NULL(energy);

    DEBUG_TRACE_I(_EXPR_, _MODULE_, "VirtualDriver creation");
	vd = new VirtualAMDriver(VERS_METERING_VIRTUALAM_NAME);
	TEST_ASSERT_NOT_NULL(vd);

	DEBUG_TRACE_I(_EXPR_, _MODULE_, "AMManager creation");
	energy = new AMManager(vd, fs, true);
	MBED_ASSERT(energy);
	energy->setJSONSupport(true);
    energy->setPublicationBase("energy");
    energy->setSubscriptionBase("energy");
    while(!energy->ready()){
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Waiting AMManager ready...");
		Thread::wait(1000);
	}
	TEST_ASSERT_TRUE(energy->ready());
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "AMManager OK!");
}
#endif


//---------------------------------------------------------------------------
/**
 * @brief Se verifica la creaci�n del objeto y la suscripci�n a topics
 * MQLib
 */
#if defined(VERS_METERING_EMi10_YTL)
TEST_CASE("Create AMManager with EMi10YTL Driver.....................", "[AMManager]") {

	// ejecuta requisitos previos
	executePrerequisites();

	// verifica condiciones iniciales
	TEST_ASSERT_NULL(ytlbus);
	TEST_ASSERT_NULL(ytl);
	TEST_ASSERT_NULL(energy);

	DEBUG_TRACE_I(_EXPR_, _MODULE_, "YTLBus UART driver setup");
	static const uint8_t ytl_address[] = {0x54,0x30,0x0a,0xba,0x95,0x03};
	static const uint8_t ytl_password[] = {0x06, 0x45, 0x4d, 0x69, 0x80, 0x90, 0xa0};
	ytlbus = new YTLBus(GPIO_NUM_17, GPIO_NUM_16, ytl_address, ytl_password, 1024, ESP_LOG_DEBUG);
	MBED_ASSERT(ytlbus);
	while(!ytlbus->ready()){
		Thread::wait(100);
	}


	// Crea el driver del medidor de energ�a
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "EMi10YTL driver setup");
	AMDriver* amdrv = NULL;
	ytl = new YTLAMDriver(*ytlbus, ENERGY_DRIVER_NAME, ENERGY_DRIVER_ANALYZERS, ESP_LOG_DEBUG);
	MBED_ASSERT(ytl);
	amdrv = static_cast<AMDriver*>(ytl);
	MBED_ASSERT(amdrv);


	DEBUG_TRACE_I(_EXPR_, _MODULE_, "AMManager setup");
	energy = new AMManager(ytl, fs, true);
	MBED_ASSERT(energy);
	energy->setJSONSupport(true);
    energy->setPublicationBase("energy");
    energy->setSubscriptionBase("energy");
    while(!energy->ready()){
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Waiting AMManager ready...");
		Thread::wait(1000);
	}
	TEST_ASSERT_TRUE(energy->ready());
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "AMManager OK!");
}
#endif

//---------------------------------------------------------------------------
/**
 * @brief Se verifican las solicitudes de boot, de configuraci�n y de estado
 */
TEST_CASE("Test GetRequest of Boot, Cfg and State....................", "[AMManager]"){

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la trama de arranque
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Requesting Bot...");
	char* msg = "{}";
	MQ::ErrorResult res = MQ::MQClient::publish("get/boot/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	double count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuraci�n mediante un GetRequest
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Requesting Config...");
	Blob::GetRequest_t* greq = new Blob::GetRequest_t(1);
	TEST_ASSERT_NOT_NULL(greq);
	cJSON* jreq = JsonParser::getJsonFromObj(*greq, ObjSelectCfg);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_PrintUnformatted(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);
	delete(greq);

	res = MQ::MQClient::publish("get/cfg/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuraci�n mediante un GetRequest
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Requesting State...");
	greq = new Blob::GetRequest_t(2);
	TEST_ASSERT_NOT_NULL(greq);
	jreq = JsonParser::getJsonFromObj(*greq, ObjSelectState);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_PrintUnformatted(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);
	delete(greq);

	res = MQ::MQClient::publish("get/value/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;
}


//---------------------------------------------------------------------------
/**
 * @brief Se verifican las solicitudes de boot, de configuraci�n y de estado
 */
TEST_CASE("Test SetRequest of measures each 15 seconds...............", "[AMManager]"){

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la trama de arranque
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Requesting measure update...");
	char* msg = "{\"idTrans\":3,\"data\":{\"cfg\":{\"measPeriod\":15}}}";
	MQ::ErrorResult res = MQ::MQClient::publish("set/cfg/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	double count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

}



//
////---------------------------------------------------------------------------
///**
// * @brief Se verifican las publicaciones y suscripciones JSON, para ello el m�dulo
// * AMManager debe ser compilado con la opci�n ASTCAL_ENABLE_JSON_SUPPORT=1 o se debe
// * activar dicha capacidad mediante AMManager::setJSONSupport(true)
// */
//TEST_CASE("JSON support .........................", "[AMManager]"){
//
//	// activa soporte JSON
//	energy->setJSONSupport(true);
//	TEST_ASSERT_TRUE(energy->isJSONSupported());
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// solicita la trama de arranque
//	char* msg = "{}";
//	MQ::ErrorResult res = MQ::MQClient::publish("get/boot/energy", msg, strlen(msg)+1, &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	double count = 0;
//	do{
//		Thread::wait(100);
//		count += 0.1;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// solicita la configuraci�n mediante un GetRequest
//	Blob::GetRequest_t* greq = new Blob::GetRequest_t(1);
//	TEST_ASSERT_NOT_NULL(greq);
//	cJSON* jreq = JsonParser::getJsonFromObj(*greq);
//	TEST_ASSERT_NOT_NULL(jreq);
//	msg = cJSON_PrintUnformatted(jreq);
//	TEST_ASSERT_NOT_NULL(msg);
//	cJSON_Delete(jreq);
//	delete(greq);
//
//	res = MQ::MQClient::publish("get/cfg/energy", msg, strlen(msg)+1, &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//	Heap::memFree(msg);
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	count = 0;
//	do{
//		Thread::wait(100);
//		count += 0.1;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// solicita la configuraci�n mediante un GetRequest
//	greq = new Blob::GetRequest_t(2);
//	TEST_ASSERT_NOT_NULL(greq);
//	jreq = JsonParser::getJsonFromObj(*greq);
//	TEST_ASSERT_NOT_NULL(jreq);
//	msg = cJSON_PrintUnformatted(jreq);
//	TEST_ASSERT_NOT_NULL(msg);
//	cJSON_Delete(jreq);
//	delete(greq);
//
//	res = MQ::MQClient::publish("get/value/energy", msg, strlen(msg)+1, &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//	Heap::memFree(msg);
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	count = 0;
//	do{
//		Thread::wait(100);
//		count += 0.1;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// actualiza la configuraci�n mediante un SetRequest
//	Blob::SetRequest_t<Blob::AMCfgData_t> req;
//	req.idTrans = 3;
//	req.data.updFlagMask = Blob::EnableAMCfgUpdNotif;
//	req.data.evtFlagMask = Blob::AMInstantMeasureEvt;
//	req.data.measPeriod = 30;
//	req.data.minmaxData.voltage = {210, 245, 5};
//	req.data.minmaxData.current = {0.015, 15, 0.005};
//	req.data.minmaxData.phase = {-185, 185, 5};
//	req.data.minmaxData.pfactor = {0.8, 1.2, 0.1};
//	req.data.minmaxData.aPow = {0, 15, 0.01};
//	req.data.minmaxData.rPow = {0, 15, 0.01};
//	req.data.minmaxData.msPow = {0, 15, 0.01};
//	req.data.minmaxData.freq = {49.7, 50.3, 0.1};
//	uint16_t default_meterRegs[] = {22136,22,51326,4488,41,0,0,3221,0,3221,0,27682,24302};
//	for(int i=0;i<sizeof(default_meterRegs)/sizeof(default_meterRegs[0]);i++){
//		req.data.calibData.meterRegs[i] = default_meterRegs[i];
//	}
//	uint16_t default_measRegs[] = {22136,28595,25151,30000,0,0,0,65437,0,65505,0,47332};
//	for(int i=0;i<sizeof(default_measRegs)/sizeof(default_measRegs[0]);i++){
//		req.data.calibData.measRegs[i] = default_measRegs[i];
//	}
//
//	jreq = JsonParser::getJsonFromSetRequest(req, JsonParser::p_data);
//	TEST_ASSERT_NOT_NULL(jreq);
//	msg = cJSON_PrintUnformatted(jreq);
//	TEST_ASSERT_NOT_NULL(msg);
//	cJSON_Delete(jreq);
//
//	res = MQ::MQClient::publish("set/cfg/energy", msg, strlen(msg)+1, &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//	Heap::memFree(msg);
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	count = 0;
//	do{
//		Thread::wait(100);
//		count += 0.1;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//	energy->stopMeasureWork();
//}
//
//
////---------------------------------------------------------------------------
///**
// * @brief Se verifican las publicaciones y suscripciones JSON, para ello el m�dulo
// * AMManager debe ser compilado con la opci�n ASTCAL_ENABLE_JSON_SUPPORT=1 o se debe
// * activar dicha capacidad mediante AMManager::setJSONSupport(true)
// */
//TEST_CASE("Blob support .........................", "[AMManager]"){
//
//	// activa soporte JSON
//	energy->setJSONSupport(false);
//	TEST_ASSERT_FALSE(energy->isJSONSupported());
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// solicita la trama de arranque
//	char* msg = "{}";
//	MQ::ErrorResult res = MQ::MQClient::publish("get/boot/energy", msg, strlen(msg)+1, &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	int count = 0;
//	do{
//		Thread::wait(1000);
//		count++;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// solicita la configuraci�n mediante un GetRequest
//	Blob::GetRequest_t greq;
//	greq.idTrans = 4;
//	greq._error.code = Blob::ErrOK;
//	greq._error.descr[0] = 0;
//
//	res = MQ::MQClient::publish("get/cfg/energy", &greq, sizeof(Blob::GetRequest_t), &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	count = 0;
//	do{
//		Thread::wait(1000);
//		count++;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// solicita la configuraci�n mediante un GetRequest
//	greq.idTrans = 5;
//	greq._error.code = Blob::ErrOK;
//	greq._error.descr[0] = 0;
//
//	res = MQ::MQClient::publish("get/value/energy", &greq, sizeof(Blob::GetRequest_t), &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	count = 0;
//	do{
//		Thread::wait(1000);
//		count++;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//
//	// -----------------------------------------------
//	// borra flag de resultado
//	s_test_done = false;
//
//	// actualiza la configuraci�n mediante un SetRequest
//	Blob::SetRequest_t<Blob::AMCfgData_t> sreq;
//	sreq.idTrans = 6;
//	sreq.keys = Blob::AMKeyCfgAll;
//	sreq._error.code = Blob::ErrOK;
//	sreq._error.descr[0] = 0;
//	sreq.data.updFlagMask = Blob::EnableAMCfgUpdNotif;
//	sreq.data.evtFlagMask = Blob::AMInstantMeasureEvt;
//	sreq.data.measPeriod = 30;
//	sreq.data.minmaxData.voltage = {210, 245, 5};
//	sreq.data.minmaxData.current = {0.015, 15, 0.005};
//	sreq.data.minmaxData.phase = {-185, 185, 5};
//	sreq.data.minmaxData.pfactor = {0.8, 1.2, 0.1};
//	sreq.data.minmaxData.aPow = {0, 15, 0.01};
//	sreq.data.minmaxData.rPow = {0, 15, 0.01};
//	sreq.data.minmaxData.msPow = {0, 15, 0.01};
//	sreq.data.minmaxData.freq = {49.7, 50.3, 0.1};
//	uint16_t default_meterRegs[] = {22136,22,51326,4488,41,0,0,3221,0,3221,0,27682,24302};
//	for(int i=0;i<sizeof(default_meterRegs)/sizeof(default_meterRegs[0]);i++){
//		sreq.data.calibData.meterRegs[i] = default_meterRegs[i];
//	}
//	uint16_t default_measRegs[] = {22136,28595,25151,30000,0,0,0,65437,0,65505,0,47332};
//	for(int i=0;i<sizeof(default_measRegs)/sizeof(default_measRegs[0]);i++){
//		sreq.data.calibData.measRegs[i] = default_measRegs[i];
//	}
//
//	res = MQ::MQClient::publish("set/cfg/energy", &sreq, sizeof(Blob::SetRequest_t<Blob::AMCfgData_t>), &s_published_cb);
//	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
//
//	// wait for response at least 10 seconds, yielding this thread
//	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
//	count = 0;
//	do{
//		Thread::wait(1000);
//		count++;
//	}while(!s_test_done && count < 10);
//	TEST_ASSERT_TRUE(s_test_done);
//}
//
//------------------------------------------------------------------------------------
//-- PREREQUISITES -------------------------------------------------------------------
//------------------------------------------------------------------------------------




//------------------------------------------------------------------------------------
static void publishedCb(const char* topic, int32_t result){

}


//------------------------------------------------------------------------------------
static void subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Recibido topic %s con mensaje:", topic);
	cJSON* obj = cJSON_Parse(msg);
	// Print JSON object
	if(obj){
		cJSON_Delete(obj);
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", (char*)msg);
	}
	else{
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error procesando mensaje en topic %s", topic);
	}
	s_test_done = true;
}


//------------------------------------------------------------------------------------
static void executePrerequisites(){
	// Prerequisites execution control flag */
	static bool s_executed_prerequisites = false;

	if(!s_executed_prerequisites){

		// inicia mqlib
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Init MQLib...");
		MQ::ErrorResult res = MQ::MQBroker::start(64);
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// espera a que est� disponible
		while(!MQ::MQBroker::ready()){
			Thread::wait(100);
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "MQLib OK!");

		// registra un manejador de publicaciones com�n
		s_published_cb = callback(&publishedCb);

		// se suscribe a todas las notificaciones de estado: stat/#
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Suscription to stat/#");
		res = MQ::MQClient::subscribe("stat/#", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// inicia el subsistema NV
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Init FSManager... ");

		fs = new FSManager("fs");
		TEST_ASSERT_NOT_NULL(fs);
		while(!fs->ready()){
			Thread::wait(100);
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "FSManager OK!");

		// marca flag de prerequisitos completado
		s_executed_prerequisites = true;
	}
}


