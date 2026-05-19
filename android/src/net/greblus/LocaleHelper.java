package net.greblus;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;

import java.io.File;
import java.io.FileInputStream;
import java.util.Locale;

/**
 * Reads the in-app language preference written by the C++ side and lets
 * Activities apply it via attachBaseContext(), so Java-side resources
 * (Toasts, dialogs) follow the user's choice in AspeQt Options instead of
 * the Android system locale.
 *
 * Contract with the C++ side:
 *   - When the user changes language (or on startup), C++ writes the
 *     two-letter language code (e.g. "fr", "pl") or "auto"/"en" to
 *     <filesDir>/aspeqt_lang.txt.
 *   - "auto" -> use system locale (return unmodified context).
 *   - "en"   -> force English.
 *   - "fr", "pl", "de", "es", "ru", "sk", "tr" -> force that locale.
 */
public final class LocaleHelper {

    private static final String LANG_FILE = "aspeqt_lang.txt";

    private LocaleHelper() {}

    /** Read the language preference. Returns null if "auto" or unreadable. */
    public static String readLang(Context base) {
        File f = new File(base.getFilesDir(), LANG_FILE);
        if (!f.exists() || f.length() == 0 || f.length() > 32) return null;
        try (FileInputStream in = new FileInputStream(f)) {
            byte[] buf = new byte[(int) f.length()];
            int n = in.read(buf);
            if (n <= 0) return null;
            String s = new String(buf, 0, n, "UTF-8").trim();
            if (s.isEmpty() || s.equals("auto")) return null;
            return s;
        } catch (Exception e) {
            return null;
        }
    }

    /**
     * Get a string resource translated to the current in-app language.
     * Reads the language preference fresh on every call so messages
     * shown AFTER the user changes language pick up the new locale
     * without requiring an Activity restart.
     */
    public static String getString(Context base, int resId) {
        return wrap(base).getResources().getString(resId);
    }

    /** Wrap a base Context with the user's chosen locale. */
    public static Context wrap(Context base) {
        String lang = readLang(base);
        if (lang == null) return base; // auto -> system locale

        // Locale.forLanguageTag accepts BCP 47 tags like "fr", "pl", "pt-BR".
        // It's not deprecated (unlike the Locale(String) constructor).
        Locale locale = Locale.forLanguageTag(lang.replace('_', '-'));
        Locale.setDefault(locale);

        Resources res = base.getResources();
        Configuration cfg = new Configuration(res.getConfiguration());
        // AspeQt's minSdk is 23, so setLocale + createConfigurationContext
        // (both available since API 17) are always supported.
        cfg.setLocale(locale);
        return base.createConfigurationContext(cfg);
    }
}
