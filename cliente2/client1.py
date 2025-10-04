import socket
import threading
import tkinter as tk
from tkinter import messagebox

SERVER_HOST = "" # direccion ip del servidor
SERVER_PORT = 8080

# Estado del cliente
user_role = "Observador"
speed = 0
battery = 0
direction = "N/A"
last_error = ""

sock = None


# --- GUI ---
def update_labels():
    lbl_speed.config(text=f"Velocidad: {speed} km/h")
    lbl_battery.config(text=f"Batería: {battery}%")
    lbl_direction.config(text=f"Dirección: {direction}")
    lbl_user.config(text=f"Usuario: {user_role}")
    lbl_error.config(text=f"Error: {last_error}" if last_error else "Error: N/A")

    # Mostrar área de comandos solo si es Admin
    if user_role == "Administrador":
        frame_commands.pack(pady=10)
    else:
        frame_commands.pack_forget()

    root.after(1000, update_labels)


# --- Hilo que escucha al servidor ---
def listen_server(sock):
    global speed, battery, direction, user_role, last_error
    while True:
        try:
            msg = sock.recv(1024).decode().strip()
            if not msg:
                break

            if msg.startswith("TELEMETRY|"):
                data = msg.split("|")[1]
                parts = dict(item.split("=") for item in data.split(";"))
                speed = int(parts.get("speed", speed))
                battery = int(parts.get("bat", battery))
                direction = parts.get("dir", direction)

            elif msg.startswith("ACK|LOGIN SUCCESS"):
                user_role = "Administrador"
                last_error = ""

            elif msg.startswith("ERROR|"):
                last_error = msg.split("|", 1)[1]

            elif msg.startswith("ACK|"):
                last_error = ""

            elif msg.startswith("USERS|"):
                txt_console.insert(tk.END, f"{msg}\n")
                txt_console.see(tk.END)

        except:
            print(" Conexión cerrada por el servidor")
            break


# --- Acciones GUI ---
def login():
    global sock
    user = entry_user.get()
    password = entry_pass.get()
    credenciales = f"{user}:{password}"
    sock.sendall(f"LOGIN|{credenciales}".encode())


def send_command():
    cmd = entry_command.get().strip()
    if cmd:
        sock.sendall(f"{cmd}".encode())
        txt_console.insert(tk.END, f">> {cmd}\n")
        txt_console.see(tk.END)
        entry_command.delete(0, tk.END)


# --- Cliente principal ---
def main():
    global root, lbl_speed, lbl_battery, lbl_direction, lbl_user, lbl_error
    global entry_user, entry_pass, entry_command, frame_commands, txt_console, sock

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER_HOST, SERVER_PORT))
    print(" Conectado al servidor")

    threading.Thread(target=listen_server, args=(sock,), daemon=True).start()

    # --- GUI ---
    root = tk.Tk()
    root.title("Cliente Vehículo Autónomo")

    # --- Login ---
    tk.Label(root, text="Usuario:", font=("Arial", 12)).pack()
    entry_user = tk.Entry(root, font=("Arial", 12))
    entry_user.pack(pady=2)

    tk.Label(root, text="Contraseña:", font=("Arial", 12)).pack()
    entry_pass = tk.Entry(root, font=("Arial", 12), show="*")
    entry_pass.pack(pady=2)

    btn_login = tk.Button(root, text="Login", command=login, font=("Arial", 12))
    btn_login.pack(pady=5)

    # --- Estado ---
    lbl_user = tk.Label(root, text=f"Usuario: {user_role}", font=("Arial", 14))
    lbl_user.pack(pady=5)

    lbl_speed = tk.Label(root, text="Velocidad: 0 km/h", font=("Arial", 14))
    lbl_speed.pack(pady=5)

    lbl_battery = tk.Label(root, text="Batería: 0%", font=("Arial", 14))
    lbl_battery.pack(pady=5)

    lbl_direction = tk.Label(root, text="Dirección: N/A", font=("Arial", 14))
    lbl_direction.pack(pady=5)

    lbl_error = tk.Label(root, text="Error: N/A", font=("Arial", 14), fg="red")
    lbl_error.pack(pady=5)

    # --- Consola de comandos (solo Admin) ---
    frame_commands = tk.Frame(root)

    txt_console = tk.Text(frame_commands, width=60, height=10, font=("Consolas", 11))
    txt_console.pack(pady=5)

    entry_command = tk.Entry(frame_commands, width=40, font=("Arial", 12))
    entry_command.pack(side=tk.LEFT, padx=5)

    btn_send = tk.Button(frame_commands, text="Enviar", command=send_command, font=("Arial", 12))
    btn_send.pack(side=tk.LEFT, padx=5)

    # Actualización periódica
    update_labels()

    root.mainloop()


if __name__ == "__main__":
    main()
