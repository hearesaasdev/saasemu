package emu.saasemu.app.core

import android.view.Surface

object NativeBridge {

    init {
        System.loadLibrary("saasemu_native")
    }

    // Initialization / directories
    external fun initNative(datapath: String): Boolean
    external fun setSystemDir(path: String)

    // Core handling
    external fun loadCore(corePath: String): Boolean
    external fun loadCoreCheck(corePath: String): Boolean
    external fun unloadCore(): Boolean

    // Game handling
    external fun loadGame(path: String): Boolean

    // Emulation control
    external fun startEmulation(): Boolean
    external fun stopEmulation(): Boolean

    // Surface control
    external fun attachSurface(surface: Surface?): Boolean
    external fun detachSurface(): Boolean

    // Input
    external fun setButtonState(id: Int, pressed: Int)

    // Optional controls
    external fun setFastForward(enabled: Boolean)
    external fun rewindFrames(frames: Int)
}