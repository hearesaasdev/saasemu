package emu.saasemu.app.ui

import android.os.Bundle
import android.widget.Switch
import androidx.appcompat.app.AppCompatActivity
import emu.saasemu.app.R
import emu.saasemu.app.core.NativeBridge

class SettingsActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)

        val swFast = findViewById<Switch>(R.id.swFast)
        val swLinear = findViewById<Switch>(R.id.swLinear)
        val swRewind = findViewById<Switch>(R.id.swRewind)

        swFast.setOnCheckedChangeListener { _, isChecked ->
            NativeBridge.setFastForward(isChecked)
        }

        swLinear.setOnCheckedChangeListener { _, isChecked ->
            // Here you could set a rendering preference; we inform native if you implement setRenderingLinear
            // Example: NativeBridge.setRenderingLinear(isChecked)
        }

        swRewind.setOnCheckedChangeListener { _, isChecked ->
            // If you implement rewind enable/disable in native, call here.
        }
    }
}SCREEN, false)
        swFast.isChecked = prefs.getBoolean(KEY_FAST, false)
        swRewind.isChecked = prefs.getBoolean(KEY_REWIND, false)

        val listener = CompoundButton.OnCheckedChangeListener { _, _ ->
            prefs.edit()
                .putBoolean(KEY_LINEAR, swLinear.isChecked)
                .putBoolean(KEY_FULLSCREEN, swFull.isChecked)
                .putBoolean(KEY_FAST, swFast.isChecked)
                .putBoolean(KEY_REWIND, swRewind.isChecked)
                .apply()

            // Call native bridge when toggles change (stubs)
            NativeBridge.setFastForward(swFast.isChecked)
            if (!swLinear.isChecked) {
                // instruct rendering to use NEAREST sampling
            }
        }

        swLinear.setOnCheckedChangeListener(listener)
        swFull.setOnCheckedChangeListener(listener)
        swFast.setOnCheckedChangeListener(listener)
        swRewind.setOnCheckedChangeListener(listener)
    }
}
