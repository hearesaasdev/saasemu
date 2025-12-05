package com.saasemu.app.ui.controls

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import com.saasemu.app.core.NativeBridge

class ButtonView(context: Context, private val buttonId: String) :
    View(context) {

    private val paint = Paint().apply {
        color = Color.argb(120, 200, 50, 50)
        isAntiAlias = true
    }

    init {
        tag = buttonId
    }

    override fun onDraw(canvas: Canvas) {
        val r = width / 2f
        canvas.drawCircle(r, r, r, paint)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                NativeBridge.setButtonState(mapButton(buttonId), 1)
                paint.color = Color.argb(180, 255, 80, 80)
                invalidate()
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                NativeBridge.setButtonState(mapButton(buttonId), 0)
                paint.color = Color.argb(120, 200, 50, 50)
                invalidate()
            }
        }
        return true
    }

    private fun mapButton(id: String): Int {
        return when (id) {
            "btnA" -> 1
            "btnB" -> 2
            "btnStart" -> 3
            else -> 0
        }
    }
}