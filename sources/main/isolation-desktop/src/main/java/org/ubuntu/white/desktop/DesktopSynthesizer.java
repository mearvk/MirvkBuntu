package org.ubuntu.white.desktop;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Pos;
import javafx.scene.Cursor;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.Pane;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Full-screen Ubuntu White Linux-style desktop preview. */
public final class DesktopSynthesizer {
    private static final String MANIFEST = "desktop-preview.json";
    private static final int ICON_SIZE = 72;
    private static final double GRID_X = 190;
    private static final double GRID_Y = 125;
    private static final double MARGIN_X = 32;
    private static final double MARGIN_Y = 28;
    private static final int EXPECTED_ICONS = 13;

    private static final String[] DEFAULT_LABELS = {
        "Desktop", "Documents", "Downloads", "Music", "Pictures", "Public",
        "Templates", "Videos", "Trash", "Applications", "Computer", "Settings", "Smaug"
    };
    private static final String[] DEFAULT_SOURCES = {
        "ubuntu-white/icons/set-002/icon-001.png", "ubuntu-white/icons/set-002/icon-002.png",
        "ubuntu-white/icons/set-002/icon-003.png", "ubuntu-white/icons/set-002/icon-004.png",
        "ubuntu-white/icons/set-002/icon-005.png", "ubuntu-white/icons/set-002/icon-006.png",
        "ubuntu-white/icons/set-002/icon-007.png", "ubuntu-white/icons/set-002/icon-008.png",
        "ubuntu-white/icons/set-002/icon-009.png", "ubuntu-white/icons/set-002/icon-010.png",
        "ubuntu-white/icons/set-002/icon-011.png", "ubuntu-white/icons/set-002/icon-012.png",
        "ubuntu-white/icons/smaug/smaug-icon-001.jpeg"
    };

    private static final class DesktopApp extends Application {
        @Override public void start(Stage stage) {
            stage.setTitle("Ubuntu White — Desktop Synthesizer");
            StackPane desktop = new StackPane();
            desktop.setStyle("-fx-background-color: black;");
            buildDesktop(desktop);

            BorderPane shell = new BorderPane(desktop);
            Label top = new Label("  Applications     Files     Settings");
            top.setStyle("-fx-background-color: rgba(255,255,255,0.90); -fx-text-fill: #333333; -fx-padding: 10px 18px; -fx-font-size: 14px;");
            shell.setTop(top);
            Label bottom = new Label("  Ubuntu White • Desktop Synthesizer    •    12 Ubuntu icons + Smaug    •    Click/drag to move    •    ESC: Exit");
            bottom.setStyle("-fx-background-color: rgba(255,255,255,0.88); -fx-text-fill: #333333; -fx-padding: 8px 14px; -fx-font-size: 12px;");
            shell.setBottom(bottom);

            Scene scene = new Scene(shell, 1280, 800);
            scene.setOnKeyPressed(e -> { if (e.getCode() == KeyCode.ESCAPE) Platform.exit(); });
            stage.setScene(scene);
            stage.setFullScreenExitHint("");
            stage.setFullScreen(true);
            stage.show();
        }
    }

    private static void buildDesktop(StackPane desktop) {
        Pane icons = new Pane();
        icons.setPickOnBounds(false);
        desktop.getChildren().add(icons);
        List<IconSpec> specs = loadManifest();
        System.out.println("Ubuntu White Desktop: loaded " + specs.size() + "/" + EXPECTED_ICONS + " icon definitions");
        for (int i = 0; i < EXPECTED_ICONS; i++) icons.getChildren().add(createIcon(specs.get(i), i));
    }

    private static List<IconSpec> loadManifest() {
        String json = null;
        Path cwd = Path.of(System.getProperty("user.dir", ".")).toAbsolutePath().normalize();
        for (Path p = cwd; p != null; p = p.getParent()) {
            Path candidate = p.resolve("main/isolation-desktop/src/main/resources/" + MANIFEST);
            if (Files.isRegularFile(candidate)) { try { json = Files.readString(candidate); break; } catch (Exception ignored) {} }
            candidate = p.resolve("src/main/resources/" + MANIFEST);
            if (Files.isRegularFile(candidate)) { try { json = Files.readString(candidate); break; } catch (Exception ignored) {} }
            if (Files.isDirectory(p.resolve(".git"))) break;
        }
        if (json == null) try (InputStream in = DesktopSynthesizer.class.getResourceAsStream("/" + MANIFEST)) {
            if (in != null) json = new String(in.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
        } catch (Exception ignored) {}

        List<IconSpec> result = new ArrayList<>();
        if (json != null) {
            Pattern entry = Pattern.compile("\\{\\s*\\\"id\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"\\s*,\\s*\\\"label\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"\\s*,\\s*\\\"source\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"\\s*\\}");
            Matcher m = entry.matcher(json);
            while (m.find()) {
                String source = m.group(3);
                String lower = source.toLowerCase();
                if (lower.endsWith(".png") || lower.endsWith(".jpeg")) result.add(new IconSpec(m.group(1), m.group(2), source));
            }
        }
        if (result.size() != EXPECTED_ICONS) {
            result.clear();
            for (int i = 0; i < EXPECTED_ICONS; i++) result.add(new IconSpec("desktop-" + String.format("%03d", i + 1), DEFAULT_LABELS[i], DEFAULT_SOURCES[i]));
        }
        return result;
    }

    private static Path resolve(String source) {
        Path relative = Path.of(source);
        Path cwd = Path.of(System.getProperty("user.dir", ".")).toAbsolutePath().normalize();
        for (Path p = cwd; p != null; p = p.getParent()) {
            Path candidate = p.resolve(relative);
            if (Files.isRegularFile(candidate)) return candidate;
            candidate = p.resolve("main/" + relative);
            if (Files.isRegularFile(candidate)) return candidate;
            if (Files.isDirectory(p.resolve(".git"))) break;
        }
        Path direct = relative.toAbsolutePath().normalize();
        return Files.isRegularFile(direct) ? direct : null;
    }

    private static Image loadImage(String source) {
        Path path = resolve(source);
        if (path != null) {
            Image image = new Image(path.toUri().toString(), ICON_SIZE, ICON_SIZE, true, true, false);
            if (!image.isError()) { System.out.println("ICON " + source + " -> " + path); return image; }
            System.err.println("ICON DECODE FAILED: " + path);
        }
        try (InputStream in = DesktopSynthesizer.class.getResourceAsStream("/" + source)) {
            if (in != null) {
                Image image = new Image(in, ICON_SIZE, ICON_SIZE, true, true);
                if (!image.isError()) return image;
            }
        } catch (Exception ignored) {}
        System.err.println("ICON NOT FOUND: " + source);
        return null;
    }

    private static VBox createIcon(IconSpec spec, int index) {
        VBox box = new VBox(5);
        box.setAlignment(Pos.CENTER);
        box.setPrefSize(145, 105);
        box.setCursor(Cursor.OPEN_HAND);
        box.setPickOnBounds(true);
        Image image = loadImage(spec.source);
        if (image != null) {
            ImageView iv = new ImageView(image);
            iv.setFitWidth(ICON_SIZE); iv.setFitHeight(ICON_SIZE); iv.setPreserveRatio(true); iv.setSmooth(true); iv.setMouseTransparent(true);
            box.getChildren().add(iv);
        }
        Label label = new Label(spec.label);
        label.setStyle("-fx-text-fill:white;-fx-font-size:13px;-fx-font-weight:bold;-fx-effect:dropshadow(gaussian,black,3,0.7,0,1);");
        label.setMouseTransparent(true);
        box.getChildren().add(label);
        int columns = 6;
        box.relocate(MARGIN_X + (index % columns) * GRID_X, MARGIN_Y + (index / columns) * GRID_Y);
        installIconDrag(box);
        return box;
    }

    private static void installIconDrag(VBox icon) {
        final double[] press = new double[2], origin = new double[2];
        icon.setOnMousePressed(e -> { press[0]=e.getSceneX(); press[1]=e.getSceneY(); origin[0]=icon.getLayoutX(); origin[1]=icon.getLayoutY(); icon.setCursor(Cursor.CLOSED_HAND); icon.toFront(); e.consume(); });
        icon.setOnMouseDragged(e -> { icon.relocate(origin[0]+e.getSceneX()-press[0], origin[1]+e.getSceneY()-press[1]); e.consume(); });
        icon.setOnMouseReleased(e -> { icon.setCursor(Cursor.OPEN_HAND); e.consume(); });
    }

    private record IconSpec(String id, String label, String source) { }
    public static void main(String[] args) { Application.launch(DesktopApp.class, args); }
}
