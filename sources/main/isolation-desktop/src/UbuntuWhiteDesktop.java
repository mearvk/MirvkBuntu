import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.control.ProgressBar;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

/** Lightweight Ubuntu White desktop idiom preview. */
public final class UbuntuWhiteDesktop extends Application {
    private static final int STEPS = 6;

    @Override
    public void start(Stage stage) {
        stage.setTitle("Ubuntu White — Desktop Preview");
        stage.setScene(new Scene(new LoaderView(stage).root(), 980, 640));
        stage.show();
    }

    private static final class LoaderView {
        private final Stage stage;
        private final BorderPane root = new BorderPane();
        private final ProgressBar progress = new ProgressBar(0);
        private final Label percent = new Label("0%");
        private final Label status = new Label("Preparing desktop preview…");

        LoaderView(Stage stage) { this.stage = stage; }

        VBox root() {
            progress.setPrefWidth(520);
            percent.setStyle("-fx-font-size: 22px;");
            status.setStyle("-fx-font-size: 14px;");
            VBox box = new VBox(16, new Label("Ubuntu White"), status, progress, percent);
            box.setAlignment(Pos.CENTER);
            box.setPadding(new Insets(32));
            root.setCenter(box);
            stage.getScene();
            startLoading();
            return box;
        }

        private void startLoading() {
            Thread worker = new Thread(() -> {
                String[] phases = {"Loading GUI sources…", "Assessing desktop assets…", "Loading fonts…",
                    "Checking icons…", "Preparing desktop shell…", "Launching preview…"};
                for (int i = 0; i < STEPS; i++) {
                    final int step = i + 1;
                    final String message = phases[i];
                    try { Thread.sleep(180); } catch (InterruptedException e) {
                        Thread.currentThread().interrupt(); return;
                    }
                    Platform.runLater(() -> {
                        progress.setProgress((double) step / STEPS);
                        percent.setText((step * 100 / STEPS) + "%");
                        status.setText(message);
                    });
                }
                Platform.runLater(() -> showDesktop());
            }, "ubuntu-white-loader");
            worker.setDaemon(true);
            worker.start();
        }

        private void showDesktop() {
            BorderPane desktop = new BorderPane();
            Label top = new Label("Ubuntu White    Applications    Places    Settings");
            top.setPadding(new Insets(12));
            Label center = new Label("Ubuntu White Desktop Preview");
            center.setStyle("-fx-font-size: 30px;");
            desktop.setTop(top);
            desktop.setCenter(center);
            desktop.setStyle("-fx-background-color: white; -fx-text-fill: #303030;");
            Scene scene = stage.getScene();
            scene.setRoot(desktop);
            scene.setOnKeyPressed(e -> {
                switch (e.getCode()) {
                    case ESCAPE -> Platform.exit();
                    case TAB -> { if (e.isControlDown()) Platform.exit(); }
                    default -> { }
                }
            });
        }
    }

    public static void main(String[] args) { launch(args); }
}
