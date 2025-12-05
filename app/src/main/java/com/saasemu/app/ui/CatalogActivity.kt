package emu.saasemu.app.ui

import android.app.AlertDialog
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import emu.saasemu.app.R
import emu.saasemu.app.core.CoreStorage
import java.io.File

class CatalogActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_catalog)

        val btnRefresh = findViewById<Button>(R.id.btnRefresh)
        btnRefresh.setOnClickListener { showRoms() }

        showRoms()
    }

    private fun showRoms() {
        val romDir = CoreStorage.romsDir(this)
        val files = romDir.listFiles()?.filter { it.isFile }?.map { it.name }?.toTypedArray() ?: arrayOf()
        if (files.isEmpty()) {
            AlertDialog.Builder(this)
                .setTitle("ROMs")
                .setMessage("Nenhuma ROM encontrada em ${romDir.absolutePath}")
                .setPositiveButton("OK", null)
                .show()
            return
        }

        AlertDialog.Builder(this)
            .setTitle("Escolha ROM")
            .setItems(files) { _, which ->
                val name = files[which]
                val f = File(romDir, name)
                val i = Intent(this, EmulationActivity::class.java)
                i.putExtra("rom_path", f.absolutePath)
                startActivity(i)
            }.show()
    }
}))
        for (f in roms) if (f.isFile) items.add(f.absolutePath)
        adapter.notifyDataSetChanged()
        if (items.isEmpty()) {
            Toast.makeText(this, "Nenhuma ROM encontrada. Coloque arquivos em files/roms ou /sdcard/Download/roms", Toast.LENGTH_LONG).show()
        }
    }
}
