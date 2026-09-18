package org.ubuntu.white.desktop;

import javafx.concurrent.Worker;
import javafx.scene.Scene;
import javafx.scene.SnapshotParameters;
import javafx.scene.image.ImageView;
import javafx.scene.image.WritableImage;
import javafx.scene.layout.StackPane;
import javafx.scene.web.WebView;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.function.Consumer;

/** Dependency-free SVG renderer using the JavaFX WebKit engine. */
public final class SvgIconRenderer {
    private SvgIconRenderer() { }

    /** Loads a local SVG and asynchronously supplies a JavaFX ImageView. */
    public static void render(Path svgPath, double width, double height,
                              Consumer<ImageView> result) {
        if (svgPath == null || !Files.isRegularFile(svgPath)) {
            result.accept(null);
            return;
        }
        try {
            String svg = Files.readString(svgPath, StandardCharsets.UTF_8);
            if (!svg.contains("<svg")) throw new IOException("Not an SVG: " + svgPath);

            WebView webView = new WebView();
            webView.setPrefSize(width, height);
            StackPane host = new StackPane(webView);
            host.setPrefSize(width, height);
            new Scene(host, width, height);

            webView.getEngine().getLoadWorker().stateProperty().addListener((obs, oldState, state) -> {
                if (state == Worker.State.SUCCEEDED) {
                    WritableImage snapshot = new WritableImage((int) Math.ceil(width), (int) Math.ceil(height));
                    SnapshotParameters parameters = new SnapshotParameters();
                    // JavaFX signature is snapshot(SnapshotParameters, WritableImage).
                    webView.snapshot(parameters, snapshot);
                    ImageView image = new ImageView(snapshot);
                    image.setFitWidth(width);
                    image.setFitHeight(height);
                    image.setPreserveRatio(true);
                    result.accept(image);
                } else if (state == Worker.State.FAILED || state == Worker.State.CANCELLED) {
                    result.accept(null);
                }
            });
            webView.getEngine().loadContent(svg, "image/svg+xml");
        } catch (IOException | RuntimeException ex) {
            result.accept(null);
        }
    }
}
