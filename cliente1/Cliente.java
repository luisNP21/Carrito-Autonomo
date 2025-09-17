import java.io.*;
import java.net.*;

public class Cliente {
    public static void main(String[] args) {
        String host = "127.0.0.1";
        int port = 8080;

        try (Socket socket = new Socket(host, port)) {
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            String msg = in.readLine();
            System.out.println("Mensaje del servidor: " + msg);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
