# Proyecto de Telemetría — Vehículo Autónomo

Sistema **cliente–servidor** en **C, Python y Java** que simula la telemetría de un vehículo autónomo en tiempo real.
El **servidor** corre en **AWS EC2** y **múltiples clientes** pueden conectarse simultáneamente para **recibir telemetría** y (si son administradores) **enviar comandos** de control.

---

## Tipos de usuario

* **Administrador:** puede autenticarse, enviar comandos y consultar usuarios conectados.
* **Observador:** solo recibe los datos de telemetría emitidos por el servidor.

---

## Estructura del proyecto

```text
Carrito-Autonomo/
│
├── servidor/              # Código fuente en C (multicliente con pthread)
│   ├── main.c
│   ├── client_handler.c
│   ├── client_handler.h
│   ├── utils.c
│   ├── utils.h
│
├── cliente1/              # Cliente Python
│   └── client1.py
│
├── cliente2/              # Cliente Java
│   ├── Cliente.class
│   ├── Cliente.java
│   
│
├── logs/                  # Registros de eventos del servidor
│   └── log.txt
│
└── README.md
```

---

## Despliegue del servidor en AWS EC2

El servidor está **configurado y probado** sobre una instancia EC2 (Ubuntu/Amazon Linux).

> **Nota:** Los siguientes comandos **ya se usaron** para configurar la instancia. **No es necesario volver a ejecutarlos**.

```bash
# Actualización del sistema
sudo apt update && sudo apt upgrade -y

# Instalación de herramientas de compilación (C)
sudo apt install -y build-essential

# Instalación de Python y pip (para clientes/pruebas)
sudo apt install -y python3 python3-pip

# Instalación de Java (para cliente Java)
sudo apt install -y openjdk-17-jdk

# Instalación de Git (clonado de repositorio)
sudo apt install -y git
```

### Conexión a la instancia

Desde máquina local:

```bash
ssh -i "Carrito-key.pem" ubuntu@<PUBLIC_IP>
```

Reemplaza `\<PUBLIC_IP>` por la **IP pública actual** de la instancia.

> **Importante:** Cada vez que el servidor se reinicia, la **IP pública cambia**.
> Antes de probar o conectar clientes, **avisa para encender** la instancia y obtener la **IP actualizada**.

---

## Clonar el repositorio

Durante la configuración inicial se clonó:

```bash
git clone https://github.com/tu_usuario/Carrito-Autonomo.git
```

---

## Compilar y ejecutar el servidor

Una vez conectado por **SSH**:

```bash
cd Carrito-Autonomo/servidor
gcc main.c client_handler.c utils.c -o server -lpthread
./server 8080 ../logs/log.txt
```

Si todo sale bien, se vera algo como:

```
Servidor escuchando en puerto 8080
```

---

## Clientes del sistema

El sistema cuenta con **dos clientes** independientes: **Python** y **Java**.

### Cliente Python

**Configuración**

Edita `client1.py` y reemplaza la IP del servidor:

```python
SERVER_HOST = "<PUBLIC_IP_DEL_SERVIDOR>"
SERVER_PORT = 8080
```

**Ejecución**

En tu máquina local:

```bash
cd Carrito-Autonomo/cliente1
python3 client1.py
```



### Cliente Java

**Configuración**

Edita `Cliente.java` y reemplaza la IP del servidor:

```java
String SERVER_IP = "<PUBLIC_IP_DEL_SERVIDOR>";
int SERVER_PORT = 8080;
```

**Compilación y ejecución**

```bash
cd Carrito-Autonomo/cliente2
javac *.java
java Cliente
```


## Registros del servidor (logs)

El servidor guarda los eventos en `logs/log.txt`.

Visualización desde la instancia:

```bash
cd ~/Carrito-Autonomo/logs
cat log.txt
```

Cada línea incluye:

* **IP** y **puerto** del cliente.
* **Tipo de usuario** (Admin / Observador).
* **Mensaje recibido**.
* **Respuesta del servidor**.

**Ejemplo**

```
Mensaje de 127.0.0.1:50348 [Observador] -> LOGIN|admin:1234
Respuesta a 127.0.0.1:50348 [Admin] -> ACK|LOGIN SUCCESS
Respuesta a 127.0.0.1:50348 [Admin] -> TELEMETRY|speed=120;bat=100;temp=25;dir=N/A
```

---

## Roles y autenticación

| Rol           | Permisos                                                 | Comandos permitidos                                          |        |
| ------------- | -------------------------------------------------------- | ------------------------------------------------------------ | ------ |
| Administrador | Enviar comandos, consultar usuarios y recibir telemetría | `SPEED UP`, `SLOW DOWN`, `TURN LEFT`, `TURN RIGHT`, `REQUEST | USERS` |
| Observador    | Recibir telemetría                                       | —                                                            |        |

La autenticación se realiza por **credenciales válidas**, **no por IP**.
El administrador **conserva privilegios** incluso si **cambia su dirección** de red.

---

## Nota importante

El servidor **no permanece encendido** de forma continua.
Para realizar pruebas, **avisa** para **encender la instancia EC2**.
Una vez encendida, se proporcionará la **IP pública actual**, que se debe **configurar manualmente** en los clientes **Python** y **Java** antes de conectarse.

> La IP **cambia** automáticamente cada vez que la instancia se **apaga** y **reinicia**.

---

## Tecnologías utilizadas

* **C** (sockets Berkeley + `pthread`): servidor multicliente
* **Python 3**: cliente del sistema
* **Java 17**: cliente alternativo
* **AWS EC2** (Ubuntu/Amazon Linux): despliegue del servidor
* **Git / GitHub**: control de versiones
