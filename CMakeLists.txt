idf_component_register(	SRCS "AMManager.cpp" 
							"AMManager_Json.cpp"
							"AMManager_NVStore.cpp"
							"AMManager_StateMachine.cpp"
							"AMManager_Subscriptions.cpp"
							"metering_objects.cpp"
						INCLUDE_DIRS "."
                        REQUIRES mbed_api_esp32 AMDriver cpp_utils ActiveModule Blob
								 AstCalendar)