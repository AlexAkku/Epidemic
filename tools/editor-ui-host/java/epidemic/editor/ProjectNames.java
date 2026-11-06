package epidemic.editor;

/** Shared Windows project-name contract; independent of any window owner. */
public final class ProjectNames {
    private ProjectNames() { }
    public static boolean valid(String name) {
        return name != null && !name.isBlank() && name.length() <= 128 &&
            !name.matches(".*[\\\\/:*?\"<>|\\p{Cntrl}].*") &&
            !name.endsWith(".") && !name.endsWith(" ") && !name.equals(".") && !name.equals("..") &&
            !name.matches("(?i)(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\\..*)?");
    }
}
