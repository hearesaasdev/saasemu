package emu.saasemu.app

import android.app.Application
import com.google.android.material.color.DynamicColors

class App : Application() {
    override fun onCreate() {
        super.onCreate()

        // Ativa Material You quando disponível (Android 12+)
        DynamicColors.applyToActivitiesIfAvailable(this)
    }
}