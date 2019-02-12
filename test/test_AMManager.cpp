/*
 * test_AstCalendar.cpp
 *
 *	Test unitario para el módulo AMManager
 */


//------------------------------------------------------------------------------------
//-- TEST HEADERS --------------------------------------------------------------------
//------------------------------------------------------------------------------------

#include "unity.h"
#include "AMManager.h"

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

class VirtualDriver : public AMDriver  {
public:

    /** Constructor del interfaz
      */
	VirtualDriver() : AMDriver(){
		_ready = false;
	}

    /** Destructor por defecto
     */
    virtual ~VirtualDriver(){}


    /** Inicializa el chip de medida
     *	@param meter_cal Array de datos de calibración del medidor
     *	@param meter_cal_count Número de elementos del array anterior
     *	@param meas_cal Array de datos de calibración de la medida
     *	@param meas_cal_count Número de elementos del array anterior
     */
    virtual void initEnergyIC(uint16_t* meter_cal=NULL, uint8_t meter_cal_count=0, uint16_t* meas_cal=NULL, uint8_t meas_cal_count=0){
    	_ready = true;
    }


    /** Realiza la autocalibración con una carga dada
     *
     * @param max_curr Corriente de la carga dada en A
     * @param turnOnLoad Callback para activar la carga
     * @param turnOffLoad Callback para desactivar la carga
     * @param result Recibe los registros escritos de calibración
     * @param result_size Tamaño máximo que es posible recibir
     * @return Resultado de la calibración (>0: Ok_registros_escritos, <0: error)
     */
    virtual int32_t autoCalibration(double max_curr, Callback<void()>turnOnLoad, Callback<void()>turnOffLoad, uint16_t* result, int result_size){
    	return result_size;
    }

    /** Obtiene el estado del sistema de medida
     *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
     */
    virtual int32_t getSysStatus(uint16_t* pdata){
    	*pdata = 0;
    	return ErrOK;
    }


    /** Obtiene el estado del medidor
     *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
     */
    virtual int32_t getMeterStatus(uint16_t* pdata){
    	return ErrOK;
    }


	/** Lee el valor de la tensión de red
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t  getLineVoltage(double* pdata){
    	return ErrOK;
    }


	/** Lee el valor de la corriente de red
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getLineCurrent(double* pdata){
    	return ErrOK;
    }


	/** Lee la potencia activa
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Resultado >= 0, Error < 0
	 */
    virtual int32_t getActivePower(double* pdata){
    	return ErrOK;
    }


	/** Lee la potencia reactiva
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Resultado >= 0, Error < 0
	 */
    virtual int32_t getReactivePower(double* pdata){
    	return ErrOK;
    }


	/** Lee la frecuencia de red
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getFrequency(double* pdata){
    	return ErrOK;
    }


	/** Lee el factor de potencia
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getPowerFactor(double* pdata){
    	return ErrOK;
    }


	/** Lee el ángulo de phase V-I
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getPhase(double* pdata){
    	return ErrOK;
    }


	/** Lee la potencia aparente media
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getMeanAparentPower(double* pdata){
    	return ErrOK;
    }


	/** Lee la energía de importación
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getImportEnergy(double* pdata){
    	return ErrOK;
    }


	/** Lee la energía de exportación
	 *
     *	@param pdata Recibe el resultado
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getExportEnergy(double* pdata){
    	return ErrOK;
    }



	/** Lee el estado de inicialización del driver
	 *
	 * @return True: ready, sino False
	 */
    virtual bool ready(){
    	return _ready;
    }


	/** Lee todos los parámetros de energía
	 *
     *	@param pdata Recibe el resultado
     *	@param count Número de parámetros consecutivos a leer
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getEnergyData(uint16_t* pdata, uint8_t count){
    	return ErrOK;
    }


	/** Lee todos los parámetros de medida
	 *
     *	@param pdata Recibe el resultado
     *	@param count Número de parámetros consecutivos a leer
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getMeasurementData(uint16_t* pdata, uint8_t count){
    	return ErrOK;
    }


	/** Lee todos los parámetros de calibración del medidor
	 *
     *	@param pdata Recibe el resultado
     *	@param count Número de parámetros consecutivos a leer
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getMeterCalib(uint16_t* pdata, uint8_t count){
    	return ErrOK;
    }


	/** Lee todos los parámetros de calibración de la medida
	 *
     *	@param pdata Recibe el resultado
     *	@param count Número de parámetros consecutivos a leer
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t getMeasureCalib(uint16_t* pdata, uint8_t count){
    	return ErrOK;
    }


	/** Escribe todos los parámetros de calibración del medidor
	 *
     *	@param pdata Datos de calibración
     *	@param count Número de parámetros consecutivos a escribir
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t setMeterCalib(uint16_t* pdata, uint8_t count){
    	return ErrOK;
    }


	/** Escribe todos los parámetros de calibración de la medida
	 *
     *	@param pdata Datos de calibración
     *	@param count Número de parámetros consecutivos a escribir
	 * 	@return Código de error OK = 0, Error < 0
	 */
    virtual int32_t setMeasureCalib(uint16_t* pdata, uint8_t count){
    	return ErrOK;
    }
private:
    bool _ready;
};

static VirtualDriver* vd;


//------------------------------------------------------------------------------------
//-- TEST CASES ----------------------------------------------------------------------
//------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * @brief Se verifica la creación del objeto y la suscripción a topics
 * MQLib
 */
TEST_CASE("Init & MQLib suscription..............", "[AMManager]") {

	// ejecuta requisitos previos
	executePrerequisites();

	// crea el objeto
	TEST_ASSERT_NULL(energy);
    // Crea el gestor del calendario astronómico
    //  - Un único relé en GPIO_NUM_16, activo a nivel alto y con detección ZC en GPIO_NUM_36 en ambos flancos
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Creando driver virtual... ");
	vd = new VirtualDriver();
	TEST_ASSERT_NOT_NULL(vd);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando AMManager... ");
	energy = new AMManager(vd, fs, true);
	MBED_ASSERT(energy);
    energy->setPublicationBase("energy");
    energy->setSubscriptionBase("energy");
    while(!energy->ready()){
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Esperando a AMManager");
		Thread::wait(1000);
	}
	TEST_ASSERT_TRUE(energy->ready());
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "AMManager OK!");
}


//---------------------------------------------------------------------------
/**
 * @brief Se verifican las publicaciones y suscripciones JSON, para ello el módulo
 * AMManager debe ser compilado con la opción ASTCAL_ENABLE_JSON_SUPPORT=1 o se debe
 * activar dicha capacidad mediante AMManager::setJSONSupport(true)
 */
TEST_CASE("JSON support .........................", "[AMManager]"){

	// activa soporte JSON
	energy->setJSONSupport(true);
	TEST_ASSERT_TRUE(energy->isJSONSupported());

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la trama de arranque
	char* msg = "{}";
	MQ::ErrorResult res = MQ::MQClient::publish("get/boot/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	double count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	Blob::GetRequest_t* greq = new Blob::GetRequest_t(1);
	TEST_ASSERT_NOT_NULL(greq);
	cJSON* jreq = JsonParser::getJsonFromObj(*greq);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_Print(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);
	delete(greq);

	res = MQ::MQClient::publish("get/cfg/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	greq = new Blob::GetRequest_t(2);
	TEST_ASSERT_NOT_NULL(greq);
	jreq = JsonParser::getJsonFromObj(*greq);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_Print(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);
	delete(greq);

	res = MQ::MQClient::publish("get/value/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// actualiza la configuración mediante un SetRequest
	Blob::SetRequest_t<Blob::AMCfgData_t> req;
	req.idTrans = 3;
	req.data.updFlagMask = Blob::EnableAMCfgUpdNotif;
	req.data.evtFlagMask = Blob::AMInstantMeasureEvt;
	req.data.measPeriod = 30;
	req.data.minmaxData.voltage = {210, 245, 5};
	req.data.minmaxData.current = {0.015, 15, 0.005};
	req.data.minmaxData.phase = {-185, 185, 5};
	req.data.minmaxData.pfactor = {0.8, 1.2, 0.1};
	req.data.minmaxData.aPow = {0, 15, 0.01};
	req.data.minmaxData.rPow = {0, 15, 0.01};
	req.data.minmaxData.msPow = {0, 15, 0.01};
	req.data.minmaxData.freq = {49.7, 50.3, 0.1};
	uint16_t default_meterRegs[] = {22136,22,51326,4488,41,0,0,3221,0,3221,0,27682,24302};
	for(int i=0;i<sizeof(default_meterRegs)/sizeof(default_meterRegs[0]);i++){
		req.data.calibData.meterRegs[i] = default_meterRegs[i];
	}
	uint16_t default_measRegs[] = {22136,28595,25151,30000,0,0,0,65437,0,65505,0,47332};
	for(int i=0;i<sizeof(default_measRegs)/sizeof(default_measRegs[0]);i++){
		req.data.calibData.measRegs[i] = default_measRegs[i];
	}

	jreq = JsonParser::getJsonFromSetRequest(req, JsonParser::p_data);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_Print(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);

	res = MQ::MQClient::publish("set/cfg/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);
	energy->stopMeasureWork();
}


//---------------------------------------------------------------------------
/**
 * @brief Se verifican las publicaciones y suscripciones JSON, para ello el módulo
 * AMManager debe ser compilado con la opción ASTCAL_ENABLE_JSON_SUPPORT=1 o se debe
 * activar dicha capacidad mediante AMManager::setJSONSupport(true)
 */
TEST_CASE("Blob support .........................", "[AMManager]"){

	// activa soporte JSON
	energy->setJSONSupport(false);
	TEST_ASSERT_FALSE(energy->isJSONSupported());

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la trama de arranque
	char* msg = "{}";
	MQ::ErrorResult res = MQ::MQClient::publish("get/boot/energy", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	int count = 0;
	do{
		Thread::wait(1000);
		count++;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	Blob::GetRequest_t greq;
	greq.idTrans = 4;
	greq._error.code = Blob::ErrOK;
	greq._error.descr[0] = 0;

	res = MQ::MQClient::publish("get/cfg/energy", &greq, sizeof(Blob::GetRequest_t), &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);


	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(1000);
		count++;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	greq.idTrans = 5;
	greq._error.code = Blob::ErrOK;
	greq._error.descr[0] = 0;

	res = MQ::MQClient::publish("get/value/energy", &greq, sizeof(Blob::GetRequest_t), &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);


	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(1000);
		count++;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// actualiza la configuración mediante un SetRequest
	Blob::SetRequest_t<Blob::AMCfgData_t> sreq;
	sreq.idTrans = 6;
	sreq.keys = Blob::AMKeyCfgAll;
	sreq._error.code = Blob::ErrOK;
	sreq._error.descr[0] = 0;
	sreq.data.updFlagMask = Blob::EnableAMCfgUpdNotif;
	sreq.data.evtFlagMask = Blob::AMInstantMeasureEvt;
	sreq.data.measPeriod = 30;
	sreq.data.minmaxData.voltage = {210, 245, 5};
	sreq.data.minmaxData.current = {0.015, 15, 0.005};
	sreq.data.minmaxData.phase = {-185, 185, 5};
	sreq.data.minmaxData.pfactor = {0.8, 1.2, 0.1};
	sreq.data.minmaxData.aPow = {0, 15, 0.01};
	sreq.data.minmaxData.rPow = {0, 15, 0.01};
	sreq.data.minmaxData.msPow = {0, 15, 0.01};
	sreq.data.minmaxData.freq = {49.7, 50.3, 0.1};
	uint16_t default_meterRegs[] = {22136,22,51326,4488,41,0,0,3221,0,3221,0,27682,24302};
	for(int i=0;i<sizeof(default_meterRegs)/sizeof(default_meterRegs[0]);i++){
		sreq.data.calibData.meterRegs[i] = default_meterRegs[i];
	}
	uint16_t default_measRegs[] = {22136,28595,25151,30000,0,0,0,65437,0,65505,0,47332};
	for(int i=0;i<sizeof(default_measRegs)/sizeof(default_measRegs[0]);i++){
		sreq.data.calibData.measRegs[i] = default_measRegs[i];
	}

	res = MQ::MQClient::publish("set/cfg/energy", &sreq, sizeof(Blob::SetRequest_t<Blob::AMCfgData_t>), &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(1000);
		count++;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);
}

//------------------------------------------------------------------------------------
//-- PREREQUISITES -------------------------------------------------------------------
//------------------------------------------------------------------------------------


/** Prerequisites execution control flag */
static bool s_executed_prerequisites = false;


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
	// Decode depending on the topic
	else{
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Formando objeto JSON a partir de objeto Blob...");
		if(MQ::MQClient::isTokenRoot(topic, "stat/cfg")){
			if(msg_len == sizeof(Blob::Response_t<Blob::AMCfgData_t>)){
				cJSON* obj = JsonParser::getJsonFromResponse(*((Blob::Response_t<Blob::AMCfgData_t>*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
			else if(msg_len == sizeof(Blob::AMCfgData_t)){
				cJSON* obj = JsonParser::getJsonFromObj(*((Blob::AMCfgData_t*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}

		}
		else if(MQ::MQClient::isTokenRoot(topic, "stat/value")){
			if(msg_len == sizeof(Blob::Response_t<Blob::AMStatData_t>)){
				cJSON* obj = JsonParser::getJsonFromResponse(*((Blob::Response_t<Blob::AMStatData_t>*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
			else if(msg_len == sizeof(Blob::AMStatData_t)){
				cJSON* obj = JsonParser::getJsonFromObj(*((Blob::AMStatData_t*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
		}
		else if(MQ::MQClient::isTokenRoot(topic, "stat/boot")){
			if(msg_len == sizeof(Blob::AMBootData_t)){
				Blob::AMBootData_t* boot = (Blob::AMBootData_t*)msg;
				cJSON* obj = JsonParser::getJsonFromObj(*((Blob::AMBootData_t*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
		}
		else{
			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Error procesando mensaje en topic %s", topic);
			s_test_done = false;
			return;
		}
	}
	s_test_done = true;
}


//------------------------------------------------------------------------------------
static void executePrerequisites(){
	if(!s_executed_prerequisites){

		// inicia mqlib
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Init MQLib...");
		MQ::ErrorResult res = MQ::MQBroker::start(64);
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// espera a que esté disponible
		while(!MQ::MQBroker::ready()){
			Thread::wait(100);
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "MQLib OK!");

		// registra un manejador de publicaciones común
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


