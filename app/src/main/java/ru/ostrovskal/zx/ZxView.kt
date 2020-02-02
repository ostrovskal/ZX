package ru.ostrovskal.zx

import android.content.Context
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.os.Message
import android.view.SurfaceHolder
import android.view.View
import ru.ostrovskal.sshstd.Common.ACT_INIT_SURFACE
import ru.ostrovskal.sshstd.Common.RECEPIENT_FORM
import ru.ostrovskal.sshstd.utils.action
import ru.ostrovskal.sshstd.utils.i
import ru.ostrovskal.sshstd.utils.send
import ru.ostrovskal.sshstd.utils.toBoolean
import ru.ostrovskal.sshstd.widgets.Controller
import ru.ostrovskal.zx.ZxCommon.*
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class ZxView(context: Context) : GLSurfaceView(context) {

    private inner class ZxRender : Renderer {
        override fun onDrawFrame(gl: GL10) {
            updateState()
        }

        override fun onSurfaceChanged(gl: GL10, width: Int, height: Int) {
            GLES20.glViewport(0, 0, width, height)
            ZxWnd.zxCmd(ZX_CMD_PROPS, width, height, "")
            wnd.zxInitialize = true
        }

        override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
            ZxWnd.zxCmd(ZX_CMD_INIT_GL, 0, 0, "")
        }
    }

    // вернуть окно
    private val wnd             by lazy { context as ZxWnd }

    private inner class RunnableParams(private val msg: Message) : Runnable {

        override fun run() {
            var update = false
            when(msg.action) {
                ACT_INIT_SURFACE                                    -> {
                    update = true
                }
                ZxWnd.ZxMessages.ACT_PRESS_MAGIC.ordinal            -> {
                    ZxWnd.zxCmd(ZX_CMD_MAGIC, 0, 0, "")
                }
                ZxWnd.ZxMessages.ACT_UPDATE_FILTER.ordinal          -> {
                    ZxWnd.zxCmd(ZX_CMD_PROPS, "filter".i, 0, "")
                }
                ZxWnd.ZxMessages.ACT_RESET.ordinal                  -> {
                    ZxWnd.zxCmd(ZX_CMD_RESET, 0, 0, "")
                    update = true
                }
                ZxWnd.ZxMessages.ACT_MODEL.ordinal                  -> {
                    ZxWnd.zxCmd(ZX_CMD_MODEL, 0, 0, "")
                    update = true
                }
                ZxWnd.ZxMessages.ACT_IO_LOAD.ordinal,
                ZxWnd.ZxMessages.ACT_IO_SAVE.ordinal                -> {
                    update = io(msg.obj.toString(), msg.action == ZxWnd.ZxMessages.ACT_IO_LOAD.ordinal)
                }
            }
            if (update) {
                wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_NAME_PROG.ordinal)
            }
            msg.recycle()
        }
    }

    fun callAction(msg: Message) {
        queueEvent(RunnableParams(Message.obtain(msg)) )
    }

    init {
        setEGLContextClientVersion(2)
        setEGLConfigChooser(5, 6, 5, 0, 0, 0)
        setRenderer(ZxRender())
        debugFlags = 0//DEBUG_CHECK_GL_ERROR
        queueEvent(RunnableParams(Message.obtain().apply { what = ACT_INIT_SURFACE} ) )
    }

    // крестовина
    private val joyCross = Controller(context, R.id.joyCross, false, style_zxCross).apply {
        setControllerMap(jCross)
        controllerButtonNotify = { buttons -> ZxWnd.props[ZX_PROP_JOY_CROSS_VALUE] = buttons.toByte() }
    }

    // кнопки
    private val joyAction = Controller(context, R.id.joyAction, false, style_zxAction).apply {
        setControllerMap(jAction)
        controllerButtonNotify = { buttons -> ZxWnd.props[ZX_PROP_JOY_ACTION_VALUE] = buttons.toByte() }
    }

    override fun onDetachedFromWindow() {
        wnd.main.removeView(joyCross)
        wnd.main.removeView(joyAction)
        super.onDetachedFromWindow()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        super.surfaceChanged(holder, format, width, height)
        wnd.main.apply {
            if (indexOfChild(joyCross) == -1) {
                addView(joyCross)
                addView(joyAction)
            }
        }
        updateJoy()
    }

    private fun updateState() {
        if (wnd.zxInitialize) {
            val result = ZxWnd.zxExecute()
            if (result != 0) {
                wnd.hand?.send(RECEPIENT_FORM, result, a1 = ZxWnd.read16(ZX_CPU_PC), a2 = ZX_ALL)
            }
        }
    }

    private fun io(name: String, loading: Boolean): Boolean {
        val result = ZxWnd.zxIO(name, loading)
        if (!result) {
            // удалить из списка недавних
            wnd.openMRU(MENU_MRU_1, name, true)
            // отобразить диалог с надписью об ошибке
            wnd.hand?.send(
                RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_IO_ERROR.ordinal, a1 =
                if (loading) R.string.ioLoadError else R.string.ioSaveError
            )
        }
        return result
    }

    fun updateJoy() {
/*
        var show = !ZxWnd.props[ZX_PROP_SHOW_DEBUGGER].toBoolean
        if(show) show = ZxWnd.props[ZX_PROP_SHOW_JOY].toBoolean
*/
        val show = ZxWnd.props[ZX_PROP_SHOW_JOY].toBoolean
        val isv = if (show) View.VISIBLE else View.GONE
        val size = 80 + ZxWnd.props[ZX_PROP_JOY_SIZE].toInt() * 40
        val mx = wnd.main.measuredWidth
        val my = wnd.main.measuredHeight
        joyCross.apply {
            visibility = isv
            setSize(size, size)
            updatePosition(0, my)
        }
        joyAction.apply {
            visibility = isv
            setSize(size, size)
            updatePosition(mx, my)
        }
    }
}