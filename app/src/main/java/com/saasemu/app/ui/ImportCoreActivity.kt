package emu.saasemu.app.ui

import android.net.Uri
import android.os.Bundle
import android.widget.Button
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import emu.saasemu.app.R
import emu.saasemu.app.core.CoreStorage

class ImportCoreActivity : AppCompatActivity() {

    private val pickCore = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        uri ?: return@registerForActivityResult
        val out = CoreStorage.copyUriToFiles(this, uri, CoreStorage.coresDir(this))
        if (out != null) toast("Core importado: ${out.name}") else toast("Falha ao importar core")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_import_core)
        findViewById<Button>(R.id.btnPickCore).setOnClickListener {
            pickCore.launch(arrayOf("*/*"))
        }
    }

    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_SHORT).show()
}nClickListener { launcher.launch(arrayOf("*/*")) }
    }
}
