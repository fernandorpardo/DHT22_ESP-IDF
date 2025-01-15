set(srcs "main.c"
	"DHT22.c"
	)


idf_component_register(
	SRCS ${srcs} 
	INCLUDE_DIRS 
	"."
)
