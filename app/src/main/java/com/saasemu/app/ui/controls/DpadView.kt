package com.saasemu.app.ui.controls

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import com.saasemu.app.core.NativeBridge
import kotlin.math.*

class DpadView(context: Context, val dpadId: String) : View(context) {

    private val paint = Paint().apply {
        color = Color.argb(120, 80, 80, 200)
        isAntiAlias = true
    }

    override fun onDraw(canvas: Canvas) {
        val r = width / 2f
        canvas.drawCircle(r, r, r, paint)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val cx = width / 2f
        val cy = height / 2f
        val dx = event.x - cx
        val dy = event.y - cy

        val angle = atan2(dy, dx)
        val dist = hypot(dx, dy)

        when (event.action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                if (dist < width * 0.25f) {
                    resetDir()
                } else {
                    when {
                        angle >= -45f.toRad() && angle < 45f.toRad() -> press("right")
                        angle >= 45f.toRad() && angle < 135f.toRad() -> press("down")
                        angle >= 135f.toRad() || angle < -135f.toRad() -> press("left")
                        else -> press("up")
                    }
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                resetDir()
            }
        }
        return true
    }

    private fun Float.toRad() = this * PI.toFloat() / 180f

    private fun resetDir() {
        NativeBridge.setButtonState(10, 0)
        NativeBridge.setButtonState(11, 0)
        NativeBridge.setButtonState(12, 0)
        NativeBridge.setButtonState(13, 0)
    }

    private fun press(dir: String) {
        resetDir()
        val code = when (dir) {
            "up" -> 10
            "down" -> 11
            "left" -> 12
            "right" -> 13
            else -> return
        }
        NativeBridge.setButtonState(code, 1)
    }
}