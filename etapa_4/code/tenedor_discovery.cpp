#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <sstream>
#include <chrono>
#include <cstring>
#include <vector>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "Socket.h"

// Constantes utilizadas
#define DISCOVERY_PORT 5353   // Puerto UDP para descubrimiento
#define TCP_SERVER_PORT 8081  // Puerto TCP del servidor de figuras
#define CLIENT_HTTP_PORT 8080 // Puerto donde el cliente hace peticiones HTTP
#define TIMEOUT_RESPONSE 5    // Tiempo de espera máximo para recibir respuestas UDP
#define BROADCAST_WAIT 120    // Espera entre rondas de descubrimiento (segundos)

using namespace std;

// Lista de direcciones broadcast para descubrimiento (solo localhost en este ejemplo)
vector<string> broadcast_ips = {
    "172.16.123.15", // /28 (Profes)
    "172.16.123.31", // /28 ISLA 1
    "172.16.123.47", // /28 ISLA 2
    "172.16.123.63", // /28 ISLA 3
    "172.16.123.79", // /28 ISLA 4
    "172.16.123.95", // /28 ISLA 5
    "172.16.123.111", // /28 ISLA 6
};

// Tabla de ruteo: figura -> IP del servidor que la contiene
map<string, string> tabla_ruteo;
mutex tabla_mutex; // Protege el acceso a la tabla

// ---------------------------------------------
// Hilo unificado para escucha UDP (descubrimiento y shutdown)
// ---------------------------------------------
void unified_udp_listener() {
    Socket sock('d');
    sock.BuildSocket('d');
    
    // Configurar socket
    int yes = 1;
    setsockopt(sock.idSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    setsockopt(sock.idSocket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    sock.Bind(DISCOVERY_PORT);

    // Configurar timeout
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_RESPONSE;
    timeout.tv_usec = 0;
    setsockopt(sock.idSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    cout << "[UDP] Escuchando en puerto " << DISCOVERY_PORT << " (descubrimiento y shutdown)\n";

    while (true) {
        char buffer[512];
        sockaddr_in senderAddr{};
        size_t len = sock.recvFrom(buffer, sizeof(buffer) - 1, &senderAddr);
        
        if (len > 0) {
            buffer[len] = '\0';
            string mensaje(buffer);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(senderAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

            cout << "[UDP] Mensaje recibido de " << ipStr << ": " << mensaje << endl;

            // Manejar shutdown
            if (mensaje.rfind("Shutdown", 0) == 0) {
                istringstream iss(mensaje);
                string cmd, nombre;
                iss >> cmd >> nombre;
                
                lock_guard<mutex> lock(tabla_mutex);
                for (auto it = tabla_ruteo.begin(); it != tabla_ruteo.end();) {
                    if (it->second == ipStr) {
                        cout << "[SHUTDOWN] Eliminando figura '" << it->first << "' (Servidor: " << nombre << ")\n";
                        it = tabla_ruteo.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            // Manejar anuncios de servidores
            else if (mensaje != "GET /servers") {  // Ignorar broadcasts de otros tenedores
                istringstream iss(mensaje);
                string nombre, ip, lista;

                if (getline(iss, nombre, '|') &&
                    getline(iss, ip, '|') &&
                    getline(iss, lista)) {
                    
                    // Limpiar solo espacios alrededor del separador
                    nombre.erase(nombre.find_last_not_of(' ') + 1);
                    nombre.erase(0, nombre.find_first_not_of(' '));
                    ip.erase(ip.find_last_not_of(' ') + 1);
                    ip.erase(0, ip.find_first_not_of(' '));
                    lista.erase(lista.find_last_not_of(' ') + 1);
                    lista.erase(0, lista.find_first_not_of(' '));

                    // Validar IP
                    struct sockaddr_in tmp;
                    if (inet_pton(AF_INET, ip.c_str(), &tmp.sin_addr) != 1) {
                        cerr << "[UDP] IP inválida recibida: " << ip << endl;
                        continue;
                    }

                    istringstream figs_stream(lista);
                    string figura;
                    lock_guard<mutex> lock(tabla_mutex);
                    while (getline(figs_stream, figura, ',')) {
                        // Limpiar nombre de figura
                        figura.erase(figura.find_last_not_of(' ') + 1);
                        figura.erase(0, figura.find_first_not_of(' '));
                        
                        tabla_ruteo[figura] = ip;
                        cout << "[RUTEO] Figura '" << figura << "' registrada con IP " << ip << endl;
                    }
                }
            }
        }
    }
}

// ---------------------------------------------
// Hilo que envía solicitudes de descubrimiento
// ---------------------------------------------
void discovery_sender() {
    Socket send_sock('d');
    send_sock.BuildSocket('d');
    
    // Permitir broadcast
    int yes = 1;
    setsockopt(send_sock.idSocket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);

    while (true) {
        cout << "[DISCOVERY] Enviando solicitudes de descubrimiento\n";

        // Enviar broadcast a todas las IPs
        string mensaje = "GET /servers";
        for (const auto &ip : broadcast_ips) {
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
            send_sock.sendTo(mensaje.c_str(), mensaje.size(), &addr);
            cout << "[BROADCAST] Enviado a " << ip << endl;
        }

        this_thread::sleep_for(chrono::seconds(BROADCAST_WAIT));
    }

    send_sock.Close();
}

// ---------------------------------------------
// Función que atiende una conexión HTTP
// ---------------------------------------------
void manejar_peticion_http(VSocket *cliente) {
    char buffer[2048] = {0};
    size_t bytes = cliente->Read(buffer, sizeof(buffer) - 1);
    buffer[bytes] = '\0';

    string request(buffer);
    string prefix = "GET /figure?name=";
    size_t pos = request.find(prefix);

    if (pos != string::npos) {
        // Extraer el nombre de la figura
        string nombre_figura = request.substr(pos + prefix.length());
        size_t fin = nombre_figura.find(' ');
        if (fin != string::npos) {
            nombre_figura = nombre_figura.substr(0, fin);
        }

        // Limpiar caracteres no deseados
        nombre_figura.erase(remove(nombre_figura.begin(), nombre_figura.end(), '\r'), nombre_figura.end());
        nombre_figura.erase(remove(nombre_figura.begin(), nombre_figura.end(), '\n'), nombre_figura.end());

        cout << "[HTTP] Solicitud para figura: " << nombre_figura << endl;

        string ip_destino;
        {
            lock_guard<mutex> lock(tabla_mutex);
            auto it = tabla_ruteo.find(nombre_figura);
            if (it != tabla_ruteo.end()) {
                ip_destino = it->second;
            }
        }

        if (!ip_destino.empty()) {
            try {
                Socket servidor_tcp('s');
                servidor_tcp.BuildSocket('s');
                
                // Configurar timeout
                struct timeval timeout;
                timeout.tv_sec = TIMEOUT_RESPONSE;
                timeout.tv_usec = 0;
                setsockopt(servidor_tcp.idSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

                cout << "[HTTP] Conectando con servidor " << ip_destino << " para figura " << nombre_figura << endl;
                servidor_tcp.MakeConnection(ip_destino.c_str(), TCP_SERVER_PORT);

                // Solicitar la figura
                string solicitud = "GET /figure/" + nombre_figura + "\r\n";
                servidor_tcp.Write(solicitud.c_str(), solicitud.size());

                char respuesta[512];
                string figura_completa;
                while(bytes > 0) {
                    memset(respuesta, 0, sizeof(respuesta));
                    size_t bytes = servidor_tcp.Read(respuesta, sizeof(respuesta) - 1);
                    figura_completa.append(respuesta, bytes);
                    if (bytes < sizeof(respuesta, bytes) - 1) break;
                }
                
                if (figura_completa.empty()) {
                    throw runtime_error("No se recibió respuesta del servidor");
                }
                respuesta[bytes] = '\0';

                // Construir respuesta HTTP
                string body = "<html><body><pre>\n" + figura_completa + "\n</pre></body></html>";
                string http_response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n"
                    "Content-Length: " +
                    to_string(body.size()) + "\r\n\r\n" + body;

                cliente->Write(http_response.c_str(), http_response.size());
                cout << "[HTTP] Figura '" << nombre_figura << "' enviada al cliente\n";
                servidor_tcp.Close();
            }
            catch (const exception &e) {
                cerr << "[HTTP] Error al contactar servidor: " << e.what() << endl;
                
                // Eliminar figura de la tabla si falla la conexión
                lock_guard<mutex> lock(tabla_mutex);
                tabla_ruteo.erase(nombre_figura);

                string err = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                cliente->Write(err.c_str(), err.size());
            }
        } else {
            string err = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            cliente->Write(err.c_str(), err.size());
            cout << "[HTTP] Figura '" << nombre_figura << "' no encontrada\n";
        }
    }
    else if (request.find("GET /list") != string::npos) {
        // Construir lista de figuras disponibles
        string body = "<html><body><h2>Figuras disponibles:</h2><ul>";
        {
            lock_guard<mutex> lock(tabla_mutex);
            for (const auto &par : tabla_ruteo) {
                body += "<li>" + par.first + " (Servidor: " + par.second + ")</li>";
            }
        }
        body += "</ul></body></html>";

        string http_response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: " +
            to_string(body.size()) + "\r\n\r\n" + body;

        cliente->Write(http_response.c_str(), http_response.size());
        cout << "[HTTP] Lista de figuras enviada\n";
    }
    else {
        string err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        cliente->Write(err.c_str(), err.size());
        cout << "[HTTP] Solicitud inválida recibida\n";
    }

    cliente->Close();
    delete cliente;
}

// ---------------------------------------------
// Hilo HTTP que atiende a múltiples clientes
// ---------------------------------------------
void atender_clientes_http() {
    VSocket *servidor = new Socket('s');
    servidor->Bind(CLIENT_HTTP_PORT);
    servidor->MarkPassive(5);
    cout << "[HTTP] Servidor escuchando en puerto " << CLIENT_HTTP_PORT << "\n";

    while (true) {
        VSocket *cliente = servidor->AcceptConnection();
        thread(manejar_peticion_http, cliente).detach();
    }

    delete servidor;
}

// ---------------------------------------------
// Función principal
// ---------------------------------------------
int main() {
    cout << "[INIT] Tenedor de figuras iniciado\n";
    cout << " - Puerto HTTP: " << CLIENT_HTTP_PORT << "\n";
    cout << " - Puerto descubrimiento: " << DISCOVERY_PORT << "\n";

    // Iniciar hilos
    thread t1(unified_udp_listener);   // Escucha UDP unificada
    thread t2(discovery_sender);       // Envía broadcasts de descubrimiento
    thread t3(atender_clientes_http);  // Servidor HTTP

    t1.join();
    t2.join();
    t3.join();

    return 0;
}