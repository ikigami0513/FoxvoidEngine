package com.example.foxvoidandroidtemplate

import android.app.NativeActivity
import android.os.Bundle
import android.system.Os
import android.util.Log
import java.io.File
import java.io.FileOutputStream

class MainActivity : NativeActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        Log.d("Foxvoid", "Starting Stealth Bootstrapper...")
        
        val pythonHome = File(filesDir, "python_home")
        
        // 1. Extract Python standard library from the APK assets to physical storage
        if (!pythonHome.exists() || pythonHome.listFiles()?.isEmpty() == true) {
            Log.d("Foxvoid", "Extracting Python standard library to physical disk...")
            // We copy the "python_home" asset folder directly to the root of filesDir
            copyAssetFolder("python_home", filesDir)
        } else {
            Log.d("Foxvoid", "Python library already extracted.")
        }

        // 2. Tell Python exactly where to find its files
        Os.setenv("PYTHONHOME", pythonHome.absolutePath, true)
        Log.d("Foxvoid", "PYTHONHOME set to: ${pythonHome.absolutePath}")

        // 3. Force load the libraries in the correct order
        System.loadLibrary("python3.10")
        System.loadLibrary("FoxvoidStandalone")
        Log.d("Foxvoid", "Native libraries loaded. Handing over to C++ Engine.")

        // 4. Hand over control to Raylib
        super.onCreate(savedInstanceState)
    }

    // --- Utility functions to unpack assets ---

    private fun copyAssetFolder(assetPath: String, targetDir: File) {
        val assetsList = assets.list(assetPath) ?: return
        
        if (assetsList.isEmpty()) {
            // It's a file, copy it
            copySingleAsset(assetPath, targetDir)
        } else {
            // It's a directory, recursively copy its contents
            for (item in assetsList) {
                val newAssetPath = if (assetPath.isEmpty()) item else "$assetPath/$item"
                copyAssetFolder(newAssetPath, targetDir)
            }
        }
    }

    private fun copySingleAsset(assetFilePath: String, targetBaseDir: File) {
        try {
            assets.open(assetFilePath).use { inStream ->
                val outFile = File(targetBaseDir, assetFilePath)
                outFile.parentFile?.mkdirs()
                FileOutputStream(outFile).use { outStream ->
                    val buffer = ByteArray(4096)
                    var read: Int
                    while (inStream.read(buffer).also { read = it } != -1) {
                        outStream.write(buffer, 0, read)
                    }
                }
            }
        } catch (e: Exception) {
            Log.e("Foxvoid", "Failed to copy asset: $assetFilePath", e)
        }
    }
}
