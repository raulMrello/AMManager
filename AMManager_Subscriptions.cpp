/*
 * AMManager.cpp
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 */

#include "AMManager.h"


//------------------------------------------------------------------------------------
static const char* _MODULE_ = "[AMM]...........";
#define _EXPR_	(!IS_ISR())



//------------------------------------------------------------------------------------
void AMManager::subscriptionCb(const char* topic, void* msg, uint16_t msg_len){

    // si es un comando para actualizar los parámetros minmax...
    if(MQ::MQClient::isTokenRoot(topic, "set/cfg")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::SetRequest_t<metering_manager>* req = NULL;
        bool json_decoded = false;
		if(_json_supported){
			req = (Blob::SetRequest_t<metering_manager>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<metering_manager>));
			MBED_ASSERT(req);
			if(!(json_decoded = JsonParser::getSetRequestFromJson(*req, *(cJSON**)msg))){
				Heap::memFree(req);
				DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_JSON. Decodificando el mensaje");
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::SetRequest_t<metering_manager>)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        if(!json_decoded){
			// el mensaje es un blob tipo metering_manager
			req = (Blob::SetRequest_t<metering_manager>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<metering_manager>));
			MBED_ASSERT(req);
			*req = *((Blob::SetRequest_t<metering_manager>*)msg);
        }
        op->sig = RecvCfgSet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar la configuración
    if(MQ::MQClient::isTokenRoot(topic, "get/cfg")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::GetRequest_t* req = NULL;
        bool json_decoded = false;
        if(_json_supported){
			req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
			MBED_ASSERT(req);
			if(!(json_decoded = JsonParser::getGetRequestFromJson(*req, *(cJSON**)msg))){
				Heap::memFree(req);
			}
        }

        // Antes de nada, chequea que el tamaño de la zona horaria es correcto, en caso contrario, descarta el topic
        if(!json_decoded && msg_len != sizeof(Blob::GetRequest_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
        if(!json_decoded){
        	req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
        	MBED_ASSERT(req);
        	*req = *((Blob::GetRequest_t*)msg);
        }
		op->sig = RecvCfgGet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar los datos de arranque
    if(MQ::MQClient::isTokenRoot(topic, "get/boot")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        op->sig = RecvBootGet;
		// apunta a los datos
		op->msg = NULL;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar el estado
    if(MQ::MQClient::isTokenRoot(topic, "get/value")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::GetRequest_t* req = NULL;
        bool json_decoded = false;
        if(_json_supported){
			req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
			MBED_ASSERT(req);
			if(!(json_decoded = JsonParser::getGetRequestFromJson(*req, *(cJSON**)msg))){
				Heap::memFree(req);
			}
        }

        // Antes de nada, chequea que el tamaño de la zona horaria es correcto, en caso contrario, descarta el topic
        if(!json_decoded && msg_len != sizeof(Blob::GetRequest_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
        if(!json_decoded){
        	req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
        	MBED_ASSERT(req);
        	*req = *((Blob::GetRequest_t*)msg);
        }
		op->sig = RecvStatGet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para ajustar el % de activación de la carga
    if(MQ::MQClient::isTokenRoot(topic, "set/load")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::AMLoadData_t* load = NULL;
        bool json_decoded = false;
		if(_json_supported){
			load = (Blob::AMLoadData_t*)Heap::memAlloc(sizeof(Blob::AMLoadData_t));
			MBED_ASSERT(load);
			if(!(json_decoded = JsonParser::getObjFromJson(*load, *(cJSON**)msg))){
				Heap::memFree(load);
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::AMLoadData_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::LightCfgData_t
        if(!json_decoded){
			// el mensaje es un blob tipo Blob::AMCfgData_t
        	load = (Blob::AMLoadData_t*)Heap::memAlloc(sizeof(Blob::AMLoadData_t));
			MBED_ASSERT(load);
			*load = *((Blob::AMLoadData_t*)msg);
        }
        op->sig = RecvLoadSet;
		// apunta a los datos
		op->msg = load;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar los datos de arranque
    if(MQ::MQClient::isTokenRoot(topic, "set/stop")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        op->sig = RecvStopSet;
		// apunta a los datos
		op->msg = NULL;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar los datos de arranque
    if(MQ::MQClient::isTokenRoot(topic, "set/restart")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        op->sig = RecvRestartSet;
		// apunta a los datos
		op->msg = NULL;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar que notifique la siguiente medida
    if(MQ::MQClient::isTokenRoot(topic, "set/forced-notif")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // activa flag de notificación forzada
        _forced_notification = true;
        return;
    }

    // si es un comando para solicitar que notifique la siguiente medida
    if(MQ::MQClient::isTokenRoot(topic, "set/forced-meas")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);
		op->sig = RecvForcedMeasure;
		// apunta a los datos
		op->msg = NULL;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_PUT. al procesar el topic[%s]", topic);
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_TOPIC. No se puede procesar el topic [%s]", topic);
}


