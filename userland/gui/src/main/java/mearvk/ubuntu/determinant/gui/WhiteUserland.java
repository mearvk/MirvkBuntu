package mearvk.ubuntu.determinant.gui;

import javafx.application.Application;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.stage.Stage;

import java.io.File;
import java.io.IOException;
import java.util.List;

public final class WhiteUserland extends Application {
    private static final String ROOT = System.getProperty("ubuntu.determinant.root", new File(".").getAbsolutePath());

    private record AppInfo(String name, String path, String purpose) {}

    private static final List<AppInfo> APPS = List.of(
        new AppInfo("Chromium", "userland/chromium", "Web browser userland"),
        new AppInfo("Darling", "userland/darling", "macOS userland compatibility"),
        new AppInfo("DRM", "userland/drm", "Display and graphics userland"),
        new AppInfo("IntelliJ Community", "userland/intellij-community", "Java development environment"),
        new AppInfo("Java Web Server", "userland/java-web-server", "Java server userland"),
        new AppInfo("Java", "userland/java", "Java runtime/development userland"),
        new AppInfo("JDesk", "userland/jdesk", "Desktop/userland environment"),
        new AppInfo("OpenJDK", "userland/openjdk", "OpenJDK userland"),
        new AppInfo("Semeru OpenJDK 8", "userland/semeru-openjdk-8", "Semeru Java 8 userland"),
        new AppInfo("Wine", "userland/wine", "Windows compatibility userland"),
        new AppInfo("X11", "userland/x11", "X11 graphical userland")
    );

    @Override
    public void start(Stage stage) {
        stage.setTitle("Ubuntu Determinant • Userland");
        stage.setMinWidth(900);
        stage.setMinHeight(650);

        BorderPane root = new BorderPane();
        root.getStyleClass().add("app-root");
        root.setTop(header());
        root.setCenter(appGrid());
        root.setBottom(statusBar());

        Scene scene = new Scene(root, 1180, 760, Color.WHITE);
        scene.getStylesheets().add(getClass().getResource("/white.css").toExternalForm());
        stage.setScene(scene);
        stage.show();
    }

    private Node header() {
        VBox box = new VBox(5);
        box.setPadding(new Insets(24, 28, 18, 28));
        box.getStyleClass().add("header");
        Label title = new Label("Ubuntu Determinant");
        title.getStyleClass().add("title");
        Label subtitle = new Label("Userland Applications");
        subtitle.getStyleClass().add("subtitle");
        Label rule = new Label("One clean white look-and-feel for every application.");
        rule.getStyleClass().add("rule-text");
        box.getChildren().addAll(title, subtitle, rule);
        return box;
    }

    private Node appGrid() {
        TilePane grid = new TilePane();
        grid.setPadding(new Insets(22, 28, 22, 28));
        grid.setHgap(16);
        grid.setVgap(16);
        grid.setPrefColumns(3);
        grid.setTileAlignment(Pos.TOP_LEFT);
        for (AppInfo app : APPS) grid.getChildren().add(card(app));
        ScrollPane scroll = new ScrollPane(grid);
        scroll.setFitToWidth(true);
        scroll.getStyleClass().add("scroll");
        return scroll;
    }

    private VBox card(AppInfo app) {
        VBox card = new VBox(10);
        card.setPrefWidth(340);
        card.setMinHeight(190);
        card.getStyleClass().add("card");

        HBox nameLine = new HBox(10);
        nameLine.setAlignment(Pos.CENTER_LEFT);
        Label icon = new Label("●");
        icon.getStyleClass().add("icon");
        Label name = new Label(app.name());
        name.getStyleClass().add("card-title");
        nameLine.getChildren().addAll(icon, name);

        Label purpose = new Label(app.purpose());
        purpose.setWrapText(true);
        purpose.getStyleClass().add("purpose");

        File dir = new File(ROOT, app.path());
        Label status = new Label(dir.isDirectory() ? "AVAILABLE  •  " + app.path() : "PLANNED  •  " + app.path());
        status.getStyleClass().add(dir.isDirectory() ? "status-ok" : "status-muted");

        Region spacer = new Region();
        VBox.setVgrow(spacer, Priority.ALWAYS);
        HBox buttons = new HBox(8);
        Button open = button("Open");
        Button configure = button("Configure");
        Button launch = button("Launch");
        open.setOnAction(e -> openPath(dir));
        configure.setOnAction(e -> openPath(new File(dir, "config")));
        launch.setOnAction(e -> launchApp(app));
        buttons.getChildren().addAll(open, configure, launch);

        card.getChildren().addAll(nameLine, purpose, status, spacer, buttons);
        return card;
    }

    private Button button(String text) {
        Button b = new Button(text);
        b.getStyleClass().add("action-button");
        return b;
    }

    private Node statusBar() {
        HBox bar = new HBox();
        bar.setPadding(new Insets(10, 28, 14, 28));
        bar.getStyleClass().add("status-bar");
        Label label = new Label("White UI baseline  •  JavaFX 21  •  Userland root: " + ROOT);
        label.getStyleClass().add("footer");
        bar.getChildren().add(label);
        return bar;
    }

    private void openPath(File file) {
        try {
            File target = file.exists() ? file : file.getParentFile();
            if (target == null) return;
            if (java.awt.Desktop.isDesktopSupported()) java.awt.Desktop.getDesktop().open(target);
        } catch (IOException ignored) { }
    }

    private void launchApp(AppInfo app) {
        String command = switch (app.name()) {
            case "Chromium" -> "chromium";
            case "IntelliJ Community" -> "idea";
            case "Wine" -> "winecfg";
            default -> null;
        };
        if (command == null) {
            showInfo(app.name(), "This application is currently exposed through its userland folder and configuration interface. A native launch command can be assigned when its executable contract is finalized.");
            return;
        }
        try { new ProcessBuilder(command).start(); }
        catch (IOException ex) { showInfo(app.name(), "Launch command unavailable: " + command); }
    }

    private void showInfo(String title, String message) {
        Alert alert = new Alert(Alert.AlertType.INFORMATION);
        alert.setTitle(title);
        alert.setHeaderText(title);
        alert.setContentText(message);
        alert.showAndWait();
    }

    public static void main(String[] args) { launch(args); }
}
