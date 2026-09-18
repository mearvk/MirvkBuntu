package ubuntu.white.sense;

import java.util.List;

/** Read-only schema facade for COMB metadata tooling. */
public final class Comb {
    private Comb() {}

    public static final List<String> GENERIC_RATINGS = List.of(
        "use", "age", "homo", "homotype", "use_2", "useage", "manage",
        "action", "lists", "calls", "actionsagainst", "same", "came", "come",
        "hold", "research", "archer-class", "master-manager-class", "imperial-calls"
    );

    public static boolean validSenseCount(int count) {
        return count >= 1 && count <= 3;
    }

    public static void main(String[] args) {
        System.out.println("Ubuntu White Sense v1");
        for (int i = 0; i < GENERIC_RATINGS.size(); i++)
            System.out.printf("%d: %s%n", i + 1, GENERIC_RATINGS.get(i));
    }
}
