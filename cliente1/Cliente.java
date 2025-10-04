import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;

public class TelemetryClient {
    private static final String SERVER_HOST = "";//IP SERVIDOR
    private static final int SERVER_PORT = 8080;

    private Socket socket;
    private BufferedReader reader;
    private PrintWriter writer;

    private String userRole = "Observador";
    private int speed = 0;
    private int battery = 0;
    private String direction = "N/A";
    private String lastError = "";

    // --- Componentes GUI ---
    private JFrame frame;
    private JLabel lblSpeed, lblBattery, lblDirection, lblUser, lblError;
    private JTextField txtUser, txtPass, txtCommand;
    private JTextArea console;
    private JPanel commandPanel;

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> new TelemetryClient().start());
    }

    public void start() {
        conectarServidor();
        crearGUI();
        new Thread(this::escucharServidor).start();
    }

    private void conectarServidor() {
        try {
            socket = new Socket(SERVER_HOST, SERVER_PORT);
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            writer = new PrintWriter(new OutputStreamWriter(socket.getOutputStream()), true);
            System.out.println(" Conectado al servidor " + SERVER_HOST + ":" + SERVER_PORT);
        } catch (IOException e) {
            JOptionPane.showMessageDialog(null, "Error al conectar con el servidor.", "Error", JOptionPane.ERROR_MESSAGE);
            System.exit(1);
        }
    }

    private void crearGUI() {
        frame = new JFrame("Cliente Vehículo Autónomo");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(500, 600);
        frame.setLayout(new FlowLayout(FlowLayout.CENTER, 10, 10));

        // --- Login ---
        frame.add(new JLabel("Usuario:"));
        txtUser = new JTextField(15);
        frame.add(txtUser);

        frame.add(new JLabel("Contraseña:"));
        txtPass = new JPasswordField(15);
        frame.add(txtPass);

        JButton btnLogin = new JButton("Login");
        btnLogin.addActionListener(e -> enviarLogin());
        frame.add(btnLogin);

        // --- Labels de estado ---
        lblUser = new JLabel("Usuario: " + userRole);
        lblSpeed = new JLabel("Velocidad: 0 km/h");
        lblBattery = new JLabel("Batería: 0%");
        lblDirection = new JLabel("Dirección: N/A");
        lblError = new JLabel("Error: N/A");
        lblError.setForeground(Color.RED);

        frame.add(lblUser);
        frame.add(lblSpeed);
        frame.add(lblBattery);
        frame.add(lblDirection);
        frame.add(lblError);

        // --- Panel de comandos ---
        commandPanel = new JPanel();
        commandPanel.setLayout(new GridLayout(3, 2, 5, 5));
        commandPanel.setVisible(false);

        JButton btnSpeedUp = new JButton("SPEED UP");
        JButton btnSlowDown = new JButton("SLOW DOWN");
        JButton btnTurnLeft = new JButton("TURN LEFT");
        JButton btnTurnRight = new JButton("TURN RIGHT");

        btnSpeedUp.addActionListener(e -> enviarComando("COMMAND|SPEED UP"));
        btnSlowDown.addActionListener(e -> enviarComando("COMMAND|SLOW DOWN"));
        btnTurnLeft.addActionListener(e -> enviarComando("COMMAND|TURN LEFT"));
        btnTurnRight.addActionListener(e -> enviarComando("COMMAND|TURN RIGHT"));

        commandPanel.add(btnSpeedUp);
        commandPanel.add(btnSlowDown);
        commandPanel.add(btnTurnLeft);
        commandPanel.add(btnTurnRight);

        frame.add(commandPanel);

        // --- Consola ---
        console = new JTextArea(10, 40);
        console.setEditable(false);
        JScrollPane scroll = new JScrollPane(console);
        frame.add(scroll);

        // --- Campo de texto para comandos manuales ---
        txtCommand = new JTextField(30);
        JButton btnEnviar = new JButton("Enviar");
        btnEnviar.addActionListener(e -> {
            String msg = txtCommand.getText().trim();
            if (!msg.isEmpty()) {
                enviarComando(msg);
                txtCommand.setText("");
            }
        });

        frame.add(txtCommand);
        frame.add(btnEnviar);

        // --- Temporizador para refrescar UI ---
        new Timer(1000, e -> actualizarLabels()).start();

        frame.setVisible(true);
    }

    private void enviarLogin() {
        String user = txtUser.getText().trim();
        String pass = txtPass.getText().trim();
        if (!user.isEmpty() && !pass.isEmpty()) {
            writer.print("LOGIN|" + user + ":" + pass + "\n");
            writer.flush();
        }
    }

    private void enviarComando(String cmd) {
        writer.print(cmd + "\n");
        writer.flush();
        console.append(">> " + cmd + "\n");
    }

    private void escucharServidor() {
        try {
            String msg;
            while ((msg = reader.readLine()) != null) {
                if (msg.startsWith("TELEMETRY|")) {
                    String data = msg.split("\\|")[1];
                    String[] pairs = data.split(";");
                    for (String p : pairs) {
                        String[] kv = p.split("=");
                        if (kv.length == 2) {
                            switch (kv[0]) {
                                case "speed": speed = Integer.parseInt(kv[1]); break;
                                case "bat": battery = Integer.parseInt(kv[1]); break;
                                case "dir": direction = kv[1]; break;
                            }
                        }
                    }
                } else if (msg.startsWith("ACK|LOGIN SUCCESS")) {
                    userRole = "Administrador";
                    SwingUtilities.invokeLater(() -> commandPanel.setVisible(true));
                    lastError = "";
                } else if (msg.startsWith("ERROR|")) {
                    lastError = msg.split("\\|", 2)[1];
                } else if (msg.startsWith("ACK|")) {
                    lastError = "";
                } else if (msg.startsWith("USERS|")) {
                    console.append(msg + "\n");
                }
                console.append(msg + "\n");
            }
        } catch (IOException e) {
            console.append("Conexión cerrada por el servidor\n");
        }
    }

    private void actualizarLabels() {
        lblSpeed.setText("Velocidad: " + speed + " km/h");
        lblBattery.setText("Batería: " + battery + "%");
        lblDirection.setText("Dirección: " + direction);
        lblUser.setText("Usuario: " + userRole);
        lblError.setText("Error: " + (lastError.isEmpty() ? "N/A" : lastError));
    }
}
