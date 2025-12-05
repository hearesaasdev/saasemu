package emu.saasemu.app.ui

import android.net.Uri
import android.os.Bundle
import android.widget.Button
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import emu.saasemu.app.R
import emu.saasemu.app.core.CoreStorage

class ImportBiosActivity : AppCompatActivity() {

    private val pickBios = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        uri ?: return@registerForActivityResult
        val out = CoreStorage.copyUriToFiles(this, uri, CoreStorage.biosDir(this))
        if (out != null) toast("BIOS importada: ${out.name}") else toast("Falha ao importar BIOS")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_import_bios)
        findViewById<Button>(R.id.btnPickBios).setOnClickListener {
            pickBios.launch(arrayOf("*/*"))
        }
    }

    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_SHORT).show()
}lickListener { launcher.launch(arrayOf("*/*")) }
    }
}
