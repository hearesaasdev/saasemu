package com.saasemu.app.ui.controls

import android.content.Context
import android.graphics.Color
import android.util.AttributeSet
import android.view.*
import android.widget.FrameLayout
import com.saasemu.app.core.NativeBridge
import org.json.JSONObject
import java.io.BufferedReader

class TouchControlsView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : FrameLayout(context, attrs) {

    private val buttons = mutableListOf<View>()

    init {
        isClickable = true
        isFocusable = true
        loadLayoutFromJson("default.json")
    }

    private fun loadLayoutFromJson(name: String) {
        val json = readJsonFromAssets(name) ?: return

        val obj = JSONObject(json)
        val arr = obj.getJSONArray("buttons")

        for (i in 0 until arr.length()) {
            val b = arr.getJSONObject(i)

            val id = b.getString("id")
            val type = b.getString("type")
            val x = b.getDouble("x")
            val y = b.getDouble("y")
            val size = b.getDouble("size")

            val view: View =
                if (type == "dpad") DpadView(context, id)
                else ButtonView(context, id)

            val lp = LayoutParams(
                (size * resources.displayMetrics.widthPixels).toInt(),
                (size * resources.displayMetrics.widthPixels).toInt()
            )

            lp.leftMargin = (x * width).toInt()
            lp.topMargin = (y * height).toInt()

            addView(view, lp)
            buttons.add(view)
        }

        post { reposition() }
    }

    private fun reposition() {
        for (child in buttons) {
            val tag = child.tag as? ControlTag ?: continue

            val lp = child.layoutParams as LayoutParams
            lp.leftMargin = (tag.x * width).toInt()
            lp.topMargin = (tag.y * height).toInt()
            child.layoutParams = lp
        }
    }

    private fun readJsonFromAssets(filename: String): String? {
        return try {
            val input = context.assets.open(filename)
            val reader = BufferedReader(input.reader())
            reader.readText()
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }
}

data class ControlTag(
    val id: String,
    val x: Float,
    val y: Float,
    val size: Float
)