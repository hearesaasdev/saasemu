package emu.saasemu.app.core

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream

object CoreStorage {

    fun coresDir(context: Context): File {
        val f = File(context.filesDir, "cores")
        if (!f.exists()) f.mkdirs()
        return f
    }

    fun biosDir(context: Context): File {
        val f = File(context.filesDir, "bios")
        if (!f.exists()) f.mkdirs()
        return f
    }

    fun romsDir(context: Context): File {
        val f = File(context.filesDir, "roms")
        if (!f.exists()) f.mkdirs()
        return f
    }

    /**
     * Copy a content Uri (from SAF or other providers) into the destination directory.
     * Returns the File object on success, or null on failure.
     *
     * Example: copyUriToFiles(context, uri, CoreStorage.coresDir(context))
     */
    fun copyUriToFiles(context: Context, uri: Uri, destDir: File): File? {
        try {
            val resolver: ContentResolver = context.contentResolver
            resolver.openInputStream(uri)?.use { input ->
                val name = queryName(resolver, uri) ?: "file_${System.currentTimeMillis()}"
                val outFile = File(destDir, name)
                FileOutputStream(outFile).use { fos ->
                    input.copyTo(fos)
                }
                return outFile
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return null
    }

    private fun queryName(resolver: ContentResolver, uri: Uri): String? {
        resolver.query(uri, null, null, null, null)?.use { cursor ->
            val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (cursor.moveToFirst() && nameIndex >= 0) {
                return cursor.getString(nameIndex)
            }
        }
        // fallback to last segment
        return uri.lastPathSegment
    }

    /**
     * Utility: find the first core file (.so) present in coresDir, or null.
     */
    fun findFirstCore(context: Context): File? {
        val files = coresDir(context).listFiles()?.filter { it.isFile && it.name.endsWith(".so") }
        return files?.firstOrNull()
    }

    /**
     * Utility: find a ROM file by name inside romsDir (exact match), or null.
     */
    fun findRomByName(context: Context, name: String): File? {
        val dir = romsDir(context)
        val target = File(dir, name)
        return if (target.exists() && target.isFile) target else null
    }
}