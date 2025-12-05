package emu.saasemu.app.ui

import android.content.Intent
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Button
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import emu.saasemu.app.R
import emu.saasemu.app.core.CoreStorage
import emu.saasemu.app.core.NativeBridge

class EmulationActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var surfaceView: SurfaceView
    private lateinit var btnLoadCore: Button
    private lateinit var btnLoadRom: Button
    private lateinit var btnStart: Button

    private var loadedCorePath: String? = null
    private var loadedRomPath: String? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_emulation)

        surfaceView = findViewById(R.id.surface)
        btnLoadCore = findViewById(R.id.btnLoadCore)
        btnLoadRom = findViewById(R.id.btnLoadRom)
        btnStart = findViewById(R.id.btnStart)

        surfaceView.holder.addCallback(this)

        btnLoadCore.setOnClickListener {
            // open cores dir selection via Catalog or ImportCoreActivity recommended
            startActivity(Intent(this, ImportCoreActivity::class.java))
        }
        btnLoadRom.setOnClickListener {
            startActivity(Intent(this, CatalogActivity::class.java))
        }

        btnStart.setOnClickListener {
            // If rom path passed via intent, prefer it
            intent.getStringExtra("rom_path")?.let {
                loadedRomPath = it
            }
            if (loadedCorePath == null) {
                // try to pick first core
                CoreStorage.findFirstCore(this)?.let { f -> loadedCorePath = f.absolutePath }
            }
            if (loadedCorePath == null || loadedRomPath == null) {
                toast("Core e ROM necessários. Importe-os primeiro.")
                return@setOnClickListener
            }

            NativeBridge.setSystemDir(CoreStorage.biosDir(this).absolutePath)

            if (!NativeBridge.loadCore(loadedCorePath!!)) {
                toast("Falha ao carregar core")
                return@setOnClickListener
            }

            if (!NativeBridge.attachSurface(surfaceView.holder.surface)) {
                toast("Falha ao anexar Surface")
                return@setOnClickListener
            }

            if (!NativeBridge.loadGame(loadedRomPath!!)) {
                toast("Falha ao carregar ROM")
                return@setOnClickListener
            }

            if (!NativeBridge.startEmulation()) {
                toast("Falha ao iniciar emulação")
            } else {
                toast("Emulação iniciada")
            }
        }

        // If incoming intent gives rom_path, set it
        intent.getStringExtra("rom_path")?.let {
            loadedRomPath = it
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // nothing, we attach when starting
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) { }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        NativeBridge.stopEmulation()
        NativeBridge.detachSurface()
        NativeBridge.unloadCore()
    }

    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_SHORT).show()
}