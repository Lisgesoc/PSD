#SSL_LIBS=-lssl -lcrypto
#SSL_FLAGS=-DWITH_OPENSSL

SSL_LIBS=
SSL_FLAGS=

all:	 stubs client server
stubs: wsFileServer.h
	soapcpp2 -c wsFileServer.h
client:	
	gcc $(SSL_FLAGS) -o client client.c soapC.c soapClient.c -lgsoap $(SSL_LIBS) -L$(GSOAP_LIB) -I$(GSOAP_INCLUDE)

server:	
	gcc $(SSL_FLAGS) -o server server.c soapC.c soapServer.c -lgsoap $(SSL_LIBS) -L$(GSOAP_LIB) -I$(GSOAP_INCLUDE)

clean:	
	rm client server *.xml *.nsmap *.wsdl *.xsd soapStub.h soapServerLib.c soapH.h soapServer.c soapClientLib.c soapClient.c soapC.c 
