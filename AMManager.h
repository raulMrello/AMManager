/*
 * AMManager.h
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 *
 *	AMManager es el m�dulo encargado de gestionar la medida de los par�metros el�ctricos, as� como la configuraci�n
 *	del driver asociado.
 */
 
#ifndef __AMManager__H
#define __AMManager__H

#include "mbed.h"
#include "ActiveModule.h"
#include "AMManagerBlob.h"
#include "AMDriver.h"
#include "JsonParserBlob.h"
#include "metering_objects.h"
#include "cpp_utils.h"
#include <list>

/** Flag para habilitar el soporte de objetos JSON en las suscripciones a MQLib
 *  Por defecto DESACTIVADO
 */
#define AMMANAGER_ENABLE_JSON_SUPPORT		0


   
class AMManager : public ActiveModule {
  public:

    static const uint32_t MaxNumMessages = 16;		//!< M�ximo n�mero de mensajes procesables en el Mailbox del componente
              
    /** Constructor por defecto
     * 	@param driver Driver que implementa la interfaz AMDriver
     * 	@param fs Objeto FSManager para operaciones de backup
     * 	@param defdbg Flag para habilitar depuraci�n por defecto
     */
    AMManager(AMDriver* driver, FSManager* fs, bool defdbg = false, const char* name = "AMM");

    /** Constructor por defecto
     * 	@param driver_list Lista de drivers gestionados por el componente
     * 	@param fs Objeto FSManager para operaciones de backup
     * 	@param defdbg Flag para habilitar depuraci�n por defecto
     */
    AMManager(std::list<AMDriver*> driver_list, FSManager* fs, bool defdbg = false, const char* name = "AMM");


    /** Destructor
     */
    virtual ~AMManager(){}


    /** Arranca el worker de medida con una cadencia por defecto (ver DefaultMeasurePeriod)
     */
    void startMeasureWork();


    /** Detiene el worker de medida
     *
     */
    void stopMeasureWork();

    /** Getters */
    void getLoadData(uint8_t& ld) { ld = _load_data; }
    void getBootData(metering_manager& bd) { bd = _amdata; }
    void getCfgData(metering_manager_cfg& cd)   { cd = _amdata.cfg; }


    /**
     * Activa y/o desactiva el soporte JSON
     * @param flag
     */
    void setJSONSupport(bool flag){
    	_json_supported = flag;
    }


    /**
     * Obtiene el estado del soporte JSON
     * @return
     */
    bool isJSONSupported(){
    	return _json_supported;
    }


    /**--------------------------------------------------------------------------------------
     * Actualiza el nivel de visualizaci�n de las tramas de depuraci�n
     */
    void setLoggingLevel(esp_log_level_t level);

  private:

    /** M�ximo n�mero de mensajes alojables en la cola asociada a la m�quina de estados */
    static const uint32_t MaxQueueMessages = 16;

    /** Cadencia del polling de medidas por defecto (en milisegundos) */
    static const uint32_t DefaultMeasurePeriod = 2000;

    /** Lapso de tiempo (segundos) para forzar una notificaci�n de medidas tras un cambio en la carga */
    static const uint32_t SecondsToForcedNotifOnLoadChange = 3;

    /** Flags de operaciones a realizar por la tarea */
    enum MsgEventFlags{
    	RecvCfgSet 	  		= (State::EV_RESERVED_USER << 0),  /// Flag activado al recibir mensaje en "set/cfg"
    	RecvCfgGet	  		= (State::EV_RESERVED_USER << 1),  /// Flag activado al recibir mensaje en "get/cfg"
    	RecvStatGet	  		= (State::EV_RESERVED_USER << 2),  /// Flag activado al recibir mensaje en "get/stat"
    	RecvBootGet	  		= (State::EV_RESERVED_USER << 3),  /// Flag activado al recibir mensaje en "get/boot"
    	RecvLoadSet	  		= (State::EV_RESERVED_USER << 4),  /// Flag activado al recibir mensaje en "set/load"
    	RecvStopSet	  		= (State::EV_RESERVED_USER << 5),  /// Flag activado al recibir mensaje en "set/stop"
    	RecvRestartSet	  	= (State::EV_RESERVED_USER << 6),  /// Flag activado al recibir mensaje en "set/restart"
    	RecvForcedMeasure	= (State::EV_RESERVED_USER << 7),  /// Flag activado al recibir mensaje en "set/forced-meas"
    	TimedMeasureEvt		= (State::EV_RESERVED_USER << 8),  /// Evento para realizar una medida
    };


    /** Nombre del objeto creado */
    const char* _name;

    /** Objetos metering */
    metering_manager _amdata;

    /** Timer de realizaci�n de medidas */
    Ticker _meas_tmr;

    /** Contador de medidas pendientes para notificar una medida instant�nea */
    int32_t _instant_meas_counter;

    /** Gestor de la carga activa en el medidor */
    uint8_t _load_data;

    /** Flag de control para el soporte de objetos json */
    bool _json_supported;

    /** Par�metros de calibraci�n */
    static const uint16_t _meter_cal_values[];
    static const uint16_t _meas_cal_values[];

    /** Flag para forzar la notificaci�n en la siguiente medida o la medida */
    bool _forced_notification;
    bool _forced_measure;

    /** Errores acumulados durante las medidas */
    uint8_t _acc_errors;
    static const uint8_t MaxMeasureErrors = 3;

    /** Objetos para la lectura de par�metros autom�ticos de forma peri�dica */
    struct DriverObj{
    	AMDriver* drv;
    	uint32_t cycle_ms;
    	std::list<AMDriver::AutoMeasureObj*>* measures;
    	std::list<AMDriver::AutoMeasureReading*>* readings;
    };
    /** Controladores de los chip de medida */
    std::list<DriverObj*> _driver_list;

 	/** Interfaz para manejar los eventos en la m�quina de estados por defecto
      *  @param se Evento a manejar
      *  @return State::StateResult Resultado del manejo del evento
      */
    virtual State::StateResult Init_EventHandler(State::StateEvent* se);


 	/** Callback invocada al recibir una actualizaci�n de un topic local al que est� suscrito
      *  @param topic Identificador del topic
      *  @param msg Mensaje recibido
      *  @param msg_len Tama�o del mensaje
      */
    virtual void subscriptionCb(const char* topic, void* msg, uint16_t msg_len);


 	/** Callback invocada al finalizar una publicaci�n local
      *  @param topic Identificador del topic
      *  @param result Resultado de la publicaci�n
      */
    virtual void publicationCb(const char* topic, int32_t result);


   	/** Chequea la integridad de los datos de configuraci�n <_cfg>. En caso de que algo no sea
   	 * 	coherente, restaura a los valores por defecto y graba en memoria NV.
   	 * 	@return True si la integridad es correcta, False si es incorrecta
	 */
	virtual bool checkIntegrity();


   	/** Establece la configuraci�n por defecto grab�ndola en memoria NV
	 */
	virtual void setDefaultConfig();


   	/** Recupera la configuraci�n de memoria NV
	 */
	virtual void restoreConfig();


   	/** Graba la configuraci�n en memoria NV
	 */
	virtual void saveConfig();


	/** Graba un par�metro en la memoria NV
	 * 	@param param_id Identificador del par�metro
	 * 	@param data Datos asociados
	 * 	@param size Tama�o de los datos
	 * 	@param type Tipo de los datos
	 * 	@return True: �xito, False: no se pudo recuperar
	 */
	virtual bool saveParameter(const char* param_id, void* data, size_t size, NVSInterface::KeyValueType type){
		return ActiveModule::saveParameter(param_id, data, size, type);
	}


	/** Recupera un par�metro de la memoria NV
	 * 	@param param_id Identificador del par�metro
	 * 	@param data Receptor de los datos asociados
	 * 	@param size Tama�o de los datos a recibir
	 * 	@param type Tipo de los datos
	 * 	@return True: �xito, False: no se pudo recuperar
	 */
	virtual bool restoreParameter(const char* param_id, void* data, size_t size, NVSInterface::KeyValueType type){
		return ActiveModule::restoreParameter(param_id, data, size, type);
	}


	/** Ejecuta el worker de medidas temporizado
	 */
	void eventMeasureWorkCb();


	/** Ejecuta el proceso de medida
	 * 	@param enable_notif Flag para indicar si se permite enviar notificaciones de alarma (por defecto: true, en el arranque: false
	 * 	para realizar una medida inicial y actualizar las variables a enviar en la trama de boot)
	 */
	void _measure(bool enable_notif);


	/** Chequea si hay que notificar alarmas
	 *
	 * @param alarm_notif Recibe el flag 'True' si hay que notificar, en caso contrario no se actualiza
	 * @param flags Recibe los flags actualizados
	 * @param flagmask M�scara de flags activos para la generaci�n de alarmas
	 * @param measure Par�metro de medida
	 * @param data_range Rando minmax del par�metro
	 * @param flag_over_limit Flag al salir de rango por arriba
	 * @param flag_below_limit Flag al salir de rango por abajo
	 * @param flag_in_range Flag al entrar en rango
	 */
	void alarmChecking(	bool& alarm_notif,
						uint32_t& flags,
						uint32_t flagmask,
						double measure,
						common_range_minmaxthres_double data_range,
						uint32_t flag_over_limit,
						uint32_t flag_below_limit,
						uint32_t flag_in_range);


	/** Actualiza la configuraci�n
	 *
	 * @param data Nueva configuraci�n a aplicar
	 * @param err Recibe los errores generados durante la actualizaci�n
	 */
	void _updateConfig(const metering_manager& data, Blob::ErrorData_t& err);


	/***
	 * Responde con el estado actual
	 */
	void _responseWithState(uint32_t idTrans, Blob::ErrorData_t& err);


	/***
	 * Responde con la configuraci�n actual
	 */
	void _responseWithConfig(uint32_t idTrans, Blob::ErrorData_t& err);


	/***
	 * Notifica el estado actual
	 */
	void _notifyState();

};
     
#endif /*__AMManager__H */

/**** END OF FILE ****/


