import java.io.*;
import java.net.*;
import java.util.Scanner;

public class Main {
    private static final String SERVER_HOST = "127.0.0.1";
    private static final int SERVER_PORT = 8080;

    public static void main(String[] args) {
        try {
            Socket socket = new Socket(SERVER_HOST, SERVER_PORT);
            System.out.println(" Conectado al servidor en " + SERVER_HOST + ":" + SERVER_PORT);

            // Hilos: uno para escuchar y otro para enviar
            new Thread(new ListenServer(socket)).start();
            new Thread(new SendCommands(socket)).start();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

// --- Hilo para escuchar al servidor ---
class ListenServer implements Runnable {
    private Socket socket;

    public ListenServer(Socket socket) {
        this.socket = socket;
    }

    @Override
    public void run() {
        try {
            BufferedReader reader = new BufferedReader(
                    new InputStreamReader(socket.getInputStream())
            );

            String msg;
            while ((msg = reader.readLine()) != null) {
                System.out.println(" Servidor: " + msg);
            }
        } catch (IOException e) {
            System.out.println(" Conexión cerrada por el servidor");
        }
    }
}

// --- Hilo para enviar comandos desde consola ---
class SendCommands implements Runnable {
    private Socket socket;

    public SendCommands(Socket socket) {
        this.socket = socket;
    }

    @Override
    public void run() {
        try {
            OutputStream out = socket.getOutputStream();
            PrintWriter writer = new PrintWriter(new OutputStreamWriter(out), true);
            Scanner scanner = new Scanner(System.in);

            while (true) {
                System.out.print(">> ");
                String msg = scanner.nextLine();

                if (msg.equalsIgnoreCase("exit")) {
                    writer.print("LOGOUT|\n");
                    writer.flush();
                    socket.close();
                    break;
                }

                // IMPORTANTE: usar print + flush en vez de println
                writer.print(msg + "\n");
                writer.flush();
            }
        } catch (IOException e) {
            System.out.println(" Error enviando comandos");
        }
    }
}
