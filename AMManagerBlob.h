/*
 * AMManagerBlob.h
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 *
 *	AMManagerBlob es el componente del m�dulo AMManager en el que se definen los objetos y tipos relativos a
 *	los objetos BLOB de este m�dulo.
 *	Todos los tipos definidos en este componente est�n asociados al namespace "Blob", de forma que puedan ser
 *	accesibles mediante el uso de: "Blob::"  e importando este archivo de cabecera.
 */
 
#ifndef AMManagerBlob
#define AMManagerBlob

#include "Blob.h"
#include "mbed.h"


#define AM_OBJ_VERSION_DEFAULT	""
#define AM_OBJ_VERSION_M90E26	"m90e26"

namespace Blob {


/** Flags para la configuraci�n de notificaciones de un objeto AMManager cuando su configuraci�n se ha
 *  modificado.
 */
 enum AMUpdFlags{
	 EnableAMCfgUpdNotif 		= (1 << 0),	/// Habilita notificaci�n de cambios en cualquier par�metro de la configuraci�n
	 EnableAMMinMaxCfgUpdNotif 	= (1 << 1),	/// Habilita notificaci�n de cambios en ppl:energy:analyzer:cfg:minmax
	 EnableAMCalibCfgUpdNotif 	= (1 << 2),	/// Habilita notificaci�n de cambios en ppl:energy:analyzer:cfg:calib
 };


/** Cadencia m�nima y por defecto permitida de env�o de medidas instant�neas en segundos (1min) */
static const uint32_t AMMinMeasPeriod = 10;
static const uint32_t AMDefaultMeasPeriod = 900;


/** N�mero de registros de calibraci�n para la medida y para el analizador */
static const uint16_t AMCalibRegCount = 16;


/** Tama�o m�ximo de los par�metros textuales */
static const uint16_t AMTextParamLength = 32;


/** Tama�o m�ximo del nombre de las versiones de los objetos */
static const uint8_t AMVersionLength = 16;


 /** Flags de evento que utiliza AMManager para notificar un cambio de estado o alarma
  */
 enum AMEvtFlags{
	 AMNoEvents					= 0,			//!< Flag para indicar que no hay eventos
 	 AMVoltageOverLimitEvt		= (1 << 0),		//!< Evento al superar el l�mite superior (voltaje)
	 AMVoltageBelowLimitEvt		= (1 << 1),		//!< Evento al superar el l�mite inferior (voltaje)
	 AMVoltageInRangeEvt 		= (1 << 2),		//!< Evento al volver a entrar en rango (voltaje)
	 AMCurrentOverLimitEvt		= (1 << 3),		//!< Evento al superar el l�mite superior (corriente)
	 AMCurrentBelowLimitEvt 	= (1 << 4),		//!< Evento al superar el l�mite inferior (corriente)
	 AMCurrentInRangeEvt 		= (1 << 5),		//!< Evento al volver a entrar en rango (corriente)
	 AMPhaseOverLimitEvt		= (1 << 6),		//!< Evento al superar el l�mite superior (fase)
	 AMPhaseBelowLimitEvt 		= (1 << 7),		//!< Evento al superar el l�mite inferior (fase)
	 AMPhaseInRangeEvt 			= (1 << 8),		//!< Evento al volver a entrar en rango (fase)
	 AMPFactorOverLimitEvt		= (1 << 9),		//!< Evento al superar el l�mite superior (factor de potencia)
	 AMPFactorBelowLimitEvt 	= (1 << 10),	//!< Evento al superar el l�mite inferior (factor de potencia)
	 AMPFactorInRangeEvt 		= (1 << 11),	//!< Evento al volver a entrar en rango (factor de potencia)
	 AMCosPhyOverLimitEvt		= (1 << 12),	//!< Evento al superar el l�mite superior (coseno phy)
	 AMCosPhyBelowLimitEvt 		= (1 << 13),	//!< Evento al superar el l�mite inferior (coseno phy)
	 AMCosPhyInRangeEvt 		= (1 << 14),	//!< Evento al volver a entrar en rango (coseno phy)
	 AMActPowOverLimitEvt		= (1 << 15),	//!< Evento al superar el l�mite superior (potencia activa)
	 AMActPowBelowLimitEvt 		= (1 << 16),	//!< Evento al superar el l�mite inferior (potencia activa)
	 AMActPowInRangeEvt 		= (1 << 17),	//!< Evento al volver a entrar en rango (potencia activa)
	 AMReactPowOverLimitEvt		= (1 << 18),	//!< Evento al superar el l�mite superior (potencia reactiva)
	 AMReactPowBelowLimitEvt	= (1 << 19),	//!< Evento al superar el l�mite inferior (potencia reactiva)
	 AMReactPowInRangeEvt 		= (1 << 20),	//!< Evento al volver a entrar en rango (potencia reactiva)
	 AMFrequencyOverLimitEvt	= (1 << 21),	//!< Evento al superar el l�mite superior (frecuencia)
	 AMFrequencyBelowLimitEvt	= (1 << 22),	//!< Evento al superar el l�mite inferior (frecuencia)
	 AMFrequencyInRangeEvt 		= (1 << 23),	//!< Evento al volver a entrar en rango (frecuencia)
	 AMThdAOverLimitEvt			= (1 << 24),	//!< Evento al superar el l�mite superior (THD-A)
	 AMThdABelowLimitEvt		= (1 << 25),	//!< Evento al superar el l�mite inferior (THD-A)
	 AMThdAInRangeEvt 			= (1 << 26),	//!< Evento al volver a entrar en rango (THD-A)
	 AMThdVOverLimitEvt			= (1 << 27),	//!< Evento al superar el l�mite superior (THD-V)
	 AMThdVBelowLimitEvt		= (1 << 28),	//!< Evento al superar el l�mite inferior (THD-V)
	 AMThdVInRangeEvt 			= (1 << 29),	//!< Evento al volver a entrar en rango (THD-V)
	 AMInstantMeasureEvt 		= (1 << 30),	//!< Evento al realizar una medida
	 /* elemento inv�lido */
	 AMInvalidEvt				= (1 << 31)
  };


 /** Flags para identificar cada key-value de los objetos JSON que se han modificado en un SET remoto
  */
enum AMKeyNames{
	AMKeyNone 			= 0,
 	AMKeyCfgUpd			= (1 << 0),
 	AMKeyCfgEvt			= (1 << 1),
 	AMKeyCfgMeas		= (1 << 2),
 	AMKeyCfgMnxVolt		= (1 << 3),
 	AMKeyCfgMnxCurr		= (1 << 4),
 	AMKeyCfgMnxPhase	= (1 << 5),
 	AMKeyCfgMnxPfact	= (1 << 6),
 	AMKeyCfgMnxApow		= (1 << 7),
 	AMKeyCfgMnxRpow		= (1 << 8),
 	AMKeyCfgMnxMpow		= (1 << 9),
 	AMKeyCfgMnxFreq		= (1 << 10),
 	AMKeyCfgMnxThdA		= (1 << 11),
 	AMKeyCfgMnxThdV		= (1 << 12),
 	AMKeyCfgCalMetr		= (1 << 13),
 	AMKeyCfgCalMea		= (1 << 14),
	AMKeyCfgVerbosity	= (1 << 15),
	AMKeyCfgVersion		= (1 << 16),
	//
	AMKeyCfgAll			= 0x1FFFF,
};


 /** Estructuras de datos para la configuraci�n de rangos de funcionamiento min-max
  * 	Se forma por diferentes pares min-max
  * 	@var voltage Voltaje (V)
  * 	@var current Corriente (A)
  * 	@var phase Fase (�)
  * 	@var pfactor Factor de potencia (pf)
  * 	@var aPow Potencia activa (KW)
  * 	@var rPow Potencia reactiva (KW)
  * 	@var msPow Potencia media aparente (KVA)
  * 	@var freq Frecuencia (Hz)
  */
struct __packed AMMinMax_t{
	 double min;
	 double max;
	 double thres;
 };


struct __packed AMMinMaxData_t{
	char version[AMVersionLength];
 	AMMinMax_t voltage;
 	AMMinMax_t current;
 	AMMinMax_t phase;
 	AMMinMax_t pfactor;
 	AMMinMax_t msPow;
 	AMMinMax_t aPow;
 	AMMinMax_t rPow;
 	AMMinMax_t freq;
 	AMMinMax_t thdA;
 	AMMinMax_t thdV;
};


/** Estructuras de datos para la calibraci�n
 * 	@var meterRegs Valores de calibraci�n de los registros de medidor
 * 	@var measRegs Valores de calibraci�n de los registros de medida
 */
struct __packed AMCalibData_t{
	char version[AMVersionLength];
	uint16_t meterRegs[AMCalibRegCount];
	uint16_t measRegs[AMCalibRegCount];
};


  /** Estructura de datos para la configuraci�n en bloque del objeto AMManager.
   * 	Se forma por las distintas estructuras de datos de configuraci�n
   * 	@var updFlags Flags de configuraci�n de notificaci�n de cambios de configuraci�n
   * 	@var evtFlags Flags de configuraci�n de notificaci�n de eventos
   * 	@var measPeriod Cadencia de env�o de medidas instant�neas (en segundos)
   * 	@var minmaxData Datos de rangos m�n-max.
   * 	@var calibData Datos de calibraci�n
   * 	@var verbosity Nivel de visualizaci�n de trazas de depuraci�n
   */
struct __packed AMCfgData_t{
	char version[AMVersionLength];
	AMUpdFlags updFlagMask;
	AMEvtFlags evtFlagMask;
	uint32_t measPeriod;
	AMMinMaxData_t minmaxData;
	AMCalibData_t calibData;
	esp_log_level_t verbosity;
};


/** Valores de medida de energ�a
 * 	@var active Energ�a activa en KW
 * 	@var reactive Energ�a reactiva en KW
 */
struct __packed AMEnergyValues_t{
	char version[AMVersionLength];
	int32_t active;
	int32_t reactive;
};


/** Valores de medida instant�nea
 * 	@var current Corriente RMS (Amp)
 * 	@var aPow Potencia activa (KW)
 * 	@var rPow Potencia reactiva (KW)
 * 	@var voltage Tensi�n RMS (V)
 * 	@var freq Frecuencia (Hz)
 * 	@var pfactor Factor de potencia
 * 	@var phase �ngulo de fase V-I (�)
 * 	@var msPow Potencia aparente media (KVA)
 * 	@var thdA Distorsi�n harm�nica total de corriente
 * 	@var thdV Distorsi�n harm�nicoa total de tensi�n
 */
struct __packed AMMeasureValues_t{
	char version[AMVersionLength];
	double current;
	double aPow;
	double rPow;
	double voltage;
	double freq;
	double pfactor;
	double phase;
	double msPow;
	double thdA;
	double thdV;
};


/** Estructura de estado del objeto AMManager notificada tras una medida, una alarma, evento, etc...
 * 	Se forma por las distintas estructuras de datos de configuraci�n
 * 	@var flags Flags de estado
 * 	@var energyValues Valores de medida de energ�a
 * 	@var measureValues Valores de medida de potencia
 */
struct __packed AMStatData_t{
	char version[AMVersionLength];
	uint32_t flags;
	AMEnergyValues_t energyValues;
	AMMeasureValues_t measureValues;
};


/** Estructura de datos asociado al boot
 */
struct __packed AMBootData_t{
	char version[AMVersionLength];
	AMCfgData_t cfg;
	AMStatData_t stat;
};


/** Estructura asociada al porcentaje de activaci�n de la carga del medidor de energ�a (0..100) para chequeo de alarmas
 */
struct __packed AMLoadData_t{
	uint8_t outValue;
};

}	// end namespace Blob


namespace JSON {

/**
 * Codifica la configuraci�n actual en un objeto JSON
 * @param cfg Configuraci�n
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMCfg(const Blob::AMCfgData_t& cfg);

/**
 * Codifica el estado actual en un objeto JSON
 * @param stat Estado
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMStat(const Blob::AMStatData_t& stat);

/**
 * Codifica el estado de arranque en un objeto JSON
 * @param boot Estado de arranque
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMBoot(const Blob::AMBootData_t& boot);

/**
 * Codifica el % de activaci�n en un objeto JSON
 * @param stat Estado
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMLoad(const Blob::AMLoadData_t& data);

/**
 * Decodifica el mensaje JSON en un objeto de configuraci�n
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getAMCfgFromJson(Blob::AMCfgData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de estado
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getAMStatFromJson(Blob::AMStatData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de arranque
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getAMBootFromJson(Blob::AMBootData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de % de activaci�n
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getAMLoadFromJson(Blob::AMLoadData_t &obj, cJSON* json);

}	// end namespace JSON





#endif
