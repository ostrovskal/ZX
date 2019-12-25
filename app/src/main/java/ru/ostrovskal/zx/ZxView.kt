package ru.ostrovskal.zx

import android.content.Context
import android.graphics.*
import android.os.Message
import android.view.Gravity
import android.view.SurfaceHolder
import android.view.View
import ru.ostrovskal.sshstd.Common.ACT_INIT_SURFACE
import ru.ostrovskal.sshstd.Common.RECEPIENT_FORM
import ru.ostrovskal.sshstd.Config
import ru.ostrovskal.sshstd.Surface
import ru.ostrovskal.sshstd.layouts.AbsoluteLayout
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Controller
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.ZxCommon.*

class ZxView(context: Context) : Surface(context) {

    // активная поверхность рендеринга
    private var surfaceActive: Bitmap?= null

    // поверхность рендеринга
    private var surface: Bitmap?    = null

    // поверхность рендеринга
    private val surface2: Bitmap    by lazy { surface?.run { Bitmap.createBitmap(width * 4, height * 4, Bitmap.Config.ARGB_8888) } ?: error("") }

    //
    private val rSurface2: RectF    by lazy { RectF(0f, 0f, surface2.width.toFloat(), surface2.height.toFloat()) }

    private val canvas2: Canvas     by lazy { Canvas(surface2) }

    // признак промежуточного преобразования
    private var is2x                = false

    // кэшированный размер границы
    private var sizeBorder          = -1

    // вернуть окно
    private val wnd                 by lazy { context as ZxWnd }

    // кнопка записи трассера
    private val butTracer                   = Tile(context, style_key_button).apply {
        layoutParams = AbsoluteLayout.LayoutParams(70.dp, 25.dp, 0, 0)
        visibility = View.GONE
        tileIcon = 15
        setOnClickListener {
            "tracer click $tileIcon".info()
            tileIcon = if(tileIcon == 15) 16 else 15
            //ZxWnd.zxCmd(ZX_CMD_TRACER, tileIcon and 16, 0, "")
        }
    }

    // крестовина
    private val joyCross			= Controller(context, R.id.joyCross, false, style_zxCross).apply {
        setControllerMap(jCross)
        controllerButtonNotify = { buttons -> ZxWnd.props[ZX_PROP_JOY_CROSS_VALUE] = buttons.toByte() }
    }

    // кнопки
    private val joyAction			= Controller(context, R.id.joyAction, false, style_zxAction).apply {
        setControllerMap(jAction)
        controllerButtonNotify = { buttons -> ZxWnd.props[ZX_PROP_JOY_ACTION_VALUE] = buttons.toByte() }
    }

    // Кисть для отрисовки сообщения
    private var sys 				= Paint().apply {
        color = Color.GREEN
        textSize = 22f.dp
        textAlign = Paint.Align.CENTER
        typeface = context.makeFont("normal")
        setShadowLayer(1f.dp, 2f.dp, 2f.dp, 0x0.color)
    }

    // "Пустая" кисть
    private var nil 				= Paint()

    override fun onDetachedFromWindow() {
        wnd.main.removeView(joyCross)
        wnd.main.removeView(joyAction)
        wnd.main.removeView(butTracer)
        super.onDetachedFromWindow()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        super.surfaceChanged(holder, format, width, height)
        wnd.main.apply {
            if(indexOfChild(joyCross) == -1) {
                addView(joyCross)
                addView(joyAction)
                addView(butTracer)
                updateJoy()
            }
        }
        updateJoy()
        updateFilter()
        is2x = (Config.screenHeight > 1280 || Config.screenWidth > 1280)
        isSkippedFrames = ZxWnd.props[ZX_PROP_SKIP_FRAMES].toBoolean
        frameTime = 20
        updateSurface(true)
    }

    override fun updateState() {
        if(wnd.zxInitialize) {
            val result = ZxWnd.zxExecute()
            if(result != 0) {
                wnd.hand?.send(RECEPIENT_FORM, result, a1 = ZxWnd.read16(ZX_CPU_PC), a2 = ZX_ALL)
            }
        }
    }

    override fun draw(canvas: Canvas) {
        super.draw(canvas)
        surfaceActive?.apply {
            if (is2x) {
                canvas2.drawBitmap(this, null, rSurface2, nil)
                canvas.drawBitmap(surface2, null, surfaceRect, null)
            } else {
                canvas.drawBitmap(this, null, surfaceRect, nil)
            }
        }
        if (ZxWnd.props[ZX_PROP_SHOW_FPS] != 0.toByte())
            sys.drawTextInBounds(canvas, fps.toString(), surfaceRect, Gravity.START or Gravity.TOP)
        if (ZxWnd.props[ZX_PROP_EXECUTE] == 0.toByte())
            sys.drawTextInBounds(canvas, "PAUSED", surfaceRect, Gravity.CENTER)
    }

    override fun handleMessage(msg: Message): Boolean {
        var ret = false
        when (msg.action) {
            ACT_INIT_SURFACE                                -> ret = true
            ZxWnd.ZxMessages.ACT_UPDATE_TRACER_BUTTON.ordinal-> updateTracer()
            ZxWnd.ZxMessages.ACT_UPDATE_SURFACE.ordinal     -> updateSurface(msg.arg1 != 0)
            ZxWnd.ZxMessages.ACT_UPDATE_JOY.ordinal         -> updateJoy()
            ZxWnd.ZxMessages.ACT_UPDATE_FILTER.ordinal      -> updateFilter()
            ZxWnd.ZxMessages.ACT_RESET.ordinal              -> { ZxWnd.zxCmd(ZX_CMD_RESET, 0, 0, ""); ret = true }
            ZxWnd.ZxMessages.ACT_MODEL.ordinal              -> { ZxWnd.zxCmd(ZX_CMD_MODEL, 0, 0, ""); ret = true }
            ZxWnd.ZxMessages.ACT_UPDATE_SKIP_FRAMES.ordinal -> isSkippedFrames = ZxWnd.props[ZX_PROP_SKIP_FRAMES].toBoolean
            ZxWnd.ZxMessages.ACT_IO_LOAD.ordinal,
            ZxWnd.ZxMessages.ACT_IO_SAVE.ordinal            -> ret = io(msg.obj.toString(), msg.action == ZxWnd.ZxMessages.ACT_IO_LOAD.ordinal)
        }
        if(ret) {
            val name = ZxWnd.zxPresets(ZX_CMD_PRESETS_NAME)
            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_NAME_PROG.ordinal, o = name)
        }
        return true
    }

    private fun updateTracer() {
        val check = ZxWnd.props[ZX_PROP_TRACER].toBoolean
        butTracer.visibility = if(check) View.VISIBLE else View.GONE
        if(!check) ZxWnd.zxCmd(ZX_CMD_TRACER, 0, 0, "")
    }

    private fun io(name: String, loading: Boolean): Boolean {
        val result = ZxWnd.zxIO(name, loading)
        if(!result) {
            // удалить из списка недавних
            wnd.openMRU(MENU_MRU_1, name, true)
            // отобразить диалог с надписью об ошибке
            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_IO_ERROR.ordinal, a1 =
                            if(loading) R.string.ioLoadError else R.string.ioSaveError)
        }
        return result
    }

    private fun updateSurface(force: Boolean) {
        val szBorder = ZxWnd.props[ZX_PROP_BORDER_SIZE] * 16
        if(sizeBorder != szBorder || surface == null || force) {
            sizeBorder = szBorder
            surface = Bitmap.createBitmap(256 + szBorder, 192 + szBorder, Bitmap.Config.ARGB_8888)?.apply {
                ZxWnd.zxSurface(this)
            }
        }
        surfaceActive = surface
        wnd.zxInitialize = true
    }

    private fun updateJoy() {
        val isv = if(ZxWnd.props[ZX_PROP_SHOW_JOY].toBoolean) View.VISIBLE else View.GONE
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

    private fun updateFilter() {
        nil.flags = if("filter".b) Paint.FILTER_BITMAP_FLAG else 0
    }
}