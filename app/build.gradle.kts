plugins {
    id("com.android.application")
    kotlin("android")
}

android {
    namespace = "com.saasemu.app"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.saasemu.app"
        minSdk = 29
        targetSdk = 34
        versionCode = 1
        versionName = "0.1"
        ndk {
            abiFilters += listOf("armeabi-v7a","arm64-v8a")
        }
    }

    buildFeatures {
        viewBinding = true
    }

    ndkVersion = "25.2.9519653"
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("com.google.android.material:material:1.11.0")
    implementation("androidx.activity:activity-ktx:1.8.0")
    implementation("io.coil-kt:coil:2.3.0") // thumbnails
}
