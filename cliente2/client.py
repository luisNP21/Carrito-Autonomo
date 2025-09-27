import socket
import threading
import tkinter as tk

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8080

# Estado del cliente
user_role = "Observador"  # por defecto
speed = 0
battery = 0
direction = "N/A"

# --- GUI ---
def update_labels():
    lbl_speed.config(text=f"Velocidad: {speed} km/h")
    lbl_battery.config(text=f"Batería: {battery}%")
    lbl_direction.config(text=f"Dirección: {direction}")
    lbl_user.config(text=f"Usuario: {user_role}")
    root.after(1000, update_labels)

# --- Hilo que escucha al servidor ---
def listen_server(sock):
    global speed, battery, direction, user_role
    while True:
        try:
            msg = sock.recv(1024).decode().strip()
            if not msg:
                break
            if msg.startswith("USERS|"):
                print(" Usuarios conectados:", msg)
            elif msg.startswith("ERROR|") or msg.startswith("ACK|"):
                print(msg)
            # Si es TELEMETRY no lo imprimimos para no llenar la consola


            # Procesar protocolo
            if msg.startswith("TELEMETRY|"):
                data = msg.split("|")[1]
                parts = dict(item.split("=") for item in data.split(";"))
                speed = int(parts.get("speed", speed))
                battery = int(parts.get("bat", battery))
                direction = parts.get("dir", direction)
            elif msg.startswith("ACK|LOGIN SUCCESS"):
                user_role = "Administrador"
        except:
            print(" Conexión cerrada por el servidor")
            break

# --- Hilo para entrada de comandos en consola ---
def input_commands(sock):
    while True:
        msg = input(">> ")
        if msg.lower() == "exit":
            sock.close()
            break
        sock.sendall(msg.encode())

# --- Cliente principal ---
def main():
    global root, lbl_speed, lbl_battery, lbl_direction, lbl_user

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER_HOST, SERVER_PORT))
    print(" Conectado al servidor")

    # Lanzar hilos
    threading.Thread(target=listen_server, args=(sock,), daemon=True).start()
    threading.Thread(target=input_commands, args=(sock,), daemon=True).start()

    # Interfaz gráfica
    root = tk.Tk()
    root.title("Cliente Vehículo Autónomo")

    lbl_user = tk.Label(root, text=f"Usuario: {user_role}", font=("Arial", 14))
    lbl_user.pack(pady=5)

    lbl_speed = tk.Label(root, text="Velocidad: 0 km/h", font=("Arial", 14))
    lbl_speed.pack(pady=5)

    lbl_battery = tk.Label(root, text="Batería: 0%", font=("Arial", 14))
    lbl_battery.pack(pady=5)

    lbl_direction = tk.Label(root, text="Dirección: N/A", font=("Arial", 14))
    lbl_direction.pack(pady=5)

    # Actualización periódica de labels
    update_labels()

    root.mainloop()

if __name__ == "__main__":
    main()
