package emu.saasemu.app.ui

import android.content.Intent
import android.os.Bundle
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import emu.saasemu.app.R

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        findViewById<Button>(R.id.btnCatalog).setOnClickListener {
            startActivity(Intent(this, CatalogActivity::class.java))
        }
        findViewById<Button>(R.id.btnImportCore).setOnClickListener {
            startActivity(Intent(this, ImportCoreActivity::class.java))
        }
        findViewById<Button>(R.id.btnImportBios).setOnClickListener {
            startActivity(Intent(this, ImportBiosActivity::class.java))
        }
        findViewById<Button>(R.id.btnSettings).setOnClickListener {
            startActivity(Intent(this, SettingsActivity::class.java))
        }
        findViewById<Button>(R.id.btnEmulate).setOnClickListener {
            startActivity(Intent(this, EmulationActivity::class.java))
        }
    }
}tivity::class.java))
            } else {
                // redirect user to missing step
                if (!cores) startActivity(Intent(this, ImportCoreActivity::class.java))
                else startActivity(Intent(this, ImportBiosActivity::class.java))
            }
        }
    }
}

> Note: you need viewBinding enabled — build.gradle already set viewBinding = true. Android Studio will generate ActivityMainBinding.
