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
    AMManager(AMDriver* driver, FSManager* fs, bool defdbg = false);


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


    /** Interfaz para postear un mensaje de la m�quina de estados en el Mailbox de la clase heredera
     *  @param msg Mensaje a postear
     *  @return Resultado
     */
    virtual osStatus putMessage(State::Msg *msg);

    /** Getters */
    void getLoadData(Blob::AMLoadData_t& ld) { ld = _load_data; }
    void getBootData(Blob::AMBootData_t& bd) { bd = _amdata; }
    void getCfgData(Blob::AMCfgData_t& cd)   { cd = _amdata.cfg; }


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


	/**
	 * Codifica la configuraci�n actual en un objeto JSON astcal = {...}
	 * @param cfg Configuraci�n
	 * @return Objeto JSON o NULL en caso de error
	 */
	static cJSON* encodeCfg(const Blob::AMCfgData_t& cfg);

	/**
	 * Codifica el estado actual en un objeto JSON astcal = {...}
	 * @param stat Estado
	 * @return Objeto JSON o NULL en caso de error
	 */
	static cJSON* encodeStat(const Blob::AMStatData_t& stat);

	/**
	 * Codifica el estado de arranque en un objeto JSON astcal = {...}
	 * @param boot Estado de arranque
	 * @return Objeto JSON o NULL en caso de error
	 */
	static cJSON* encodeBoot(const Blob::AMBootData_t& boot);

	/**
	 * Decodifica una operaci�n SetRequest en la que se adjunta la nueva configuraci�n a aplicar
	 * @param req Recibe el objeto decodificado
	 * @param json_data Objeto JSON recibido
	 * @return True si la decodificaci�n es correcta
	 */
	static bool decodeSetRequest(Blob::SetRequest_t<Blob::AMCfgData_t>&req, char* json_data);

	/**
	 * Codifica la configuraci�n actual en un objeto JSON solicitado previamente con un idtrans
	 * @param resp Respuesta con la configuraci�n actual
	 * @return Objeto JSON
	 */
	static cJSON* encodeCfgResponse(const Blob::Response_t<Blob::AMCfgData_t> &resp);

	/**
	 * Codifica el estado actual en un objeto JSON solicitado previamente con un idtrans
	 * @param resp Respuesta con el estado actual
	 * @return Objeto JSON
	 */
	static cJSON* encodeStatResponse(const Blob::Response_t<Blob::AMStatData_t> &resp);

  private:

    /** M�ximo n�mero de mensajes alojables en la cola asociada a la m�quina de estados */
    static const uint32_t MaxQueueMessages = 16;

    /** Cadencia del polling de medidas por defecto (en milisegundos) */
    static const uint32_t DefaultMeasurePeriod = 1000;

    /** Lapso de tiempo (segundos) para forzar una notificaci�n de medidas tras un cambio en la carga */
    static const uint32_t SecondsToForcedNotifOnLoadChange = 3;

    /** Flags de operaciones a realizar por la tarea */
    enum MsgEventFlags{
    	RecvCfgSet 	  = (State::EV_RESERVED_USER << 0),  /// Flag activado al recibir mensaje en "set/cfg"
    	RecvCfgGet	  = (State::EV_RESERVED_USER << 1),  /// Flag activado al recibir mensaje en "get/cfg"
    	RecvStatGet	  = (State::EV_RESERVED_USER << 2),  /// Flag activado al recibir mensaje en "get/stat"
    	RecvBootGet	  = (State::EV_RESERVED_USER << 3),  /// Flag activado al recibir mensaje en "get/boot"
    	RecvLoadSet	  = (State::EV_RESERVED_USER << 4),  /// Flag activado al recibir mensaje en "set/load"
    };


    /** Cola de mensajes de la m�quina de estados */
    Queue<State::Msg, MaxQueueMessages> _queue;

    /** Datos de configuraci�n y de estado */
    Blob::AMBootData_t _amdata;

    /** Timer de realizaci�n de medidas */
    RtosTimer* _meas_tmr;

    /** Contador de medidas pendientes para notificar una medida instant�nea */
    int32_t _instant_meas_counter;

    /** Controlador del chip de medida */
    AMDriver* _driver;

    /** Gestor de la carga activa en el medidor */
    Blob::AMLoadData_t _load_data;

    /** Flag de control para el soporte de objetos json */
    bool _json_supported;


    /** Interfaz para obtener un evento osEvent de la clase heredera
     *  @param msg Mensaje a postear
     */
    virtual osEvent getOsEvent();


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
	 * @param measure Par�metro de medida
	 * @param data_range Rando minmax del par�metro
	 * @param flag_over_limit Flag al salir de rango por arriba
	 * @param flag_below_limit Flag al salir de rango por abajo
	 * @param flag_in_range Flag al entrar en rango
	 */
	void alarmChecking(	bool& alarm_notif,
						double measure,
						Blob::AMMinMax_t data_range,
						uint32_t flag_over_limit,
						uint32_t flag_below_limit,
						uint32_t flag_in_range);


	/** Actualiza la configuraci�n
	 *
	 * @param cfg Nueva configuraci�n a aplicar
	 * @param keys Flags de par�metros actualizados
	 * @param err Recibe los errores generados durante la actualizaci�n
	 */
	void _updateConfig(const Blob::AMCfgData_t& cfg, uint32_t keys, Blob::ErrorData_t& err);


	/**
	 * Codifica la configuraci�n actual en un objeto JSON astcal = {...}
	 * @return Objeto JSON o NULL en caso de error
	 */
	cJSON* _encodeCfg(){
		return encodeCfg(_amdata.cfg);
	}

	/**
	 * Codifica el estado actual en un objeto JSON astcal = {...}
	 * @return Objeto JSON o NULL en caso de error
	 */
	cJSON* _encodeStat(){
		return encodeStat(_amdata.stat);
	}

	/**
	 * Codifica la informaci�n de arranque en un objeto JSON astcal = {...}
	 * @return Objeto JSON o NULL en caso de error
	 */
	cJSON* _encodeBoot(){
		return encodeBoot(_amdata);
	}

	/**
	 * Decodifica un objeto JSON del tipo: amload = {"outValue": u8} en un objeto
	 * Blob::AMLoadata_t
	 * @param result Recibe el objeto decodificado
	 * @param json Objeto json a decodificar
	 * @return True si se decodifica correctamente
	 */
	bool _decodeLoadData(Blob::AMLoadData_t& result, const char* json);
};
     
#endif /*__AMManager__H */

/**** END OF FILE ****/


