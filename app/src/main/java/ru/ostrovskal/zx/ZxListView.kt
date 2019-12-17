package ru.ostrovskal.zx

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Typeface
import android.view.MotionEvent
import android.view.View
import ru.ostrovskal.sshstd.Common.RECEPIENT_FORM
import ru.ostrovskal.sshstd.STORAGE
import ru.ostrovskal.sshstd.TileDrawable
import ru.ostrovskal.sshstd.Wnd
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.sshstd.widgets.lists.CommonRibbon
import ru.ostrovskal.zx.ZxCommon.*
import kotlin.math.abs
import kotlin.math.roundToInt
import kotlin.math.sign

class ListLayout(context: Context, private val turn: Boolean) : CommonRibbon(context, true, style_debugger_ribbon) {

    class ItemView(context: Context): Text(context, style_debugger_item) {
        init {
            id = R.id.button1
            typeface = Typeface.MONOSPACE
            setOnClickListener { notify(false) }
            setOnLongClickListener { notify(true); true }
        }

        private fun notify(long: Boolean) { (parent as? ListLayout)?.notify(this, long) }
    }

    //
    lateinit var wnd: Wnd

    // селектор для выделенного элемента списка
    @JvmField var selector: TileDrawable?     = null

    // выделенное представление
    @JvmField var selView: ItemView?   = null

    // выделенный элемент списка
    @STORAGE
    @JvmField var selItem            = 0

    // точка входа в дизасм
    @STORAGE
    @JvmField var entryPC            = 0

    // точка входа в стек
    @STORAGE
    @JvmField var entrySP            = 0

    // точка входа в данные
    @STORAGE
    private var entryData            = 0

    //
    private var mDeltaX              = 0

    //
    @JvmField var countItems        = 0

    //
    @JvmField var countData         = 0

    //
    @STORAGE
    @JvmField var mode          = ZX_DEBUGGER_MODE_PC

    // ПС элементов
    @STORAGE
    private val itemAddr            = IntArray(32) { 0 }

    init {
        selector = TileDrawable(context, style_debugger_selector)

        setOnTouchListener { _, ev ->
            mTracker.addMovement(ev)
            touch.event(ev).drag(mDragSensitive) { offs, finish ->
                mDeltaX += offs.w
                val isVert = abs(offs.h) > abs(mDeltaX)
                if (touch.isUnpressed) touch.flags = 0
                if (!isVert) {
                    if(!finish) {
                        // меняем список
                        val delta = sign(mDeltaX.toFloat()).toInt()
                        mode += delta
                        mode %= 3
                        if (mode < ZX_DEBUGGER_MODE_PC) mode = ZX_DEBUGGER_MODE_DT
                        else if (mode > ZX_DEBUGGER_MODE_DT) mode = ZX_DEBUGGER_MODE_PC
                        update(0, ZX_LIST)
                        mDeltaX = 0
                    }
                } else {
                    if(!finish) {
                        // запускаем прокрутку
                        mTouchId = touch.id
                        mTracker.computeCurrentVelocity(1000, mMaxVelocity)
                        val delta = mTracker.getYVelocity(mTouchId).toInt()
                        if (abs(delta) > mMinVelocity) mFling.start(-delta)
                    } else {
                        scrolling(offs.h / mDragSensitive.h)
                    }
                    mDeltaX = 0
                }
            }
            true
        }
    }

    override fun onInterceptTouchEvent(ev: MotionEvent): Boolean {
        var delta = 0
        touch.event(ev).drag(mDragSensitive) { offs, _ ->
            delta = offs.w or offs.h
        }
        return delta != 0
    }

    @SuppressLint("DrawAllocation")
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if(childCount < 2) {
            removeAllViews()
            repeat(countItems) { addView(ItemView(context)) }
            mCount = childCount
            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_DEBUGGER.ordinal, a1 = 0, a2 = ZX_LIST)
        }
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        val hItem = getChildAt(0).measuredHeight.toFloat()
        countItems = ((measuredHeight - verticalPadding) / hItem).roundToInt()
        // определить количество элементов
        rectWithPadding(mRectList)
        mEdgeStart = mRectList.top
        mEdgeEnd = mRectList.bottom
        countData = ((((measuredWidth - horizontalPadding) / hItem) - 8) / 2f).roundToInt()
    }

    override fun scrolling(delta: Int): Boolean {
        var finish = false
        when(mode) {
            ZX_DEBUGGER_MODE_PC -> {
                finish = ZxWnd.zxCmd(ZX_CMD_MOVE, entryPC, delta, "").toByte().toBoolean
                entryPC = ZxWnd.read16(ZX_PROP_JNI_RETURN_VALUE)
            }
            ZX_DEBUGGER_MODE_SP -> {
                entrySP -= delta * 2
                finish = correctSP()
            }
            ZX_DEBUGGER_MODE_DT -> {
                entryData -= delta * countData
                finish = correctDT()
            }
        }
        update(0, ZX_LIST)
        return finish
    }

    fun notify(item: View, long: Boolean) {
        val data = itemAddr[indexOfChild(item)]
        if(long) wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_DEBUGGER_LONG_CLOCK.ordinal, a1 = data)
        else update(data, ZX_SL)
    }

    private fun correctSP(): Boolean {
        val entry = entrySP
        val delta = entry % 2
        val count = countItems * 2
        if(entrySP < 0) entrySP = -delta
        else if((entrySP + count) > 65535)
            entrySP = (65536 + delta) - count
        return entry != entrySP
    }

    private fun correctDT(): Boolean {
        val entry = entryData
        val delta = entryData % countData
        val count = countItems * countData
        if(entryData < 0) entryData = delta
        else if((entryData + count) > 65535)
            entryData = (65536 + countData - delta) - count
        return entry != entryData
    }

    fun update(data: Int, flags: Int) {
        if(flags test ZX_REG) {
            entrySP = ZxWnd.read16(ZX_CPU_SP)
            entrySP -= (countItems and -2)
        }
        if(flags test ZX_PC) {
            // читаем текущий PC
            entryPC = data
        }
        if(flags test ZX_DT) {
            entryData = data
        }
        if(flags test ZX_SEL) {
            selItem = data
            if(itemAddr.search(data, -1) == -1) entryPC = data
        }
        if(flags test ZX_LIST) {
            show(flags)
        }
        invalidate()
    }

    private fun show(flags: Int) {
        var flg = 0
        selView?.background = null
        selView = null
        var entry = when(mode) {
            ZX_DEBUGGER_MODE_PC    -> {
                flg =   DA_PC or (ZxWnd.props[ZX_PROP_SHOW_LABEL].toInt() * DA_LABEL) or
                        (ZxWnd.props[ZX_PROP_SHOW_CODE].toInt() * DA_CODE) or
                        (ZxWnd.props[ZX_PROP_SHOW_CODE_VALUE].toInt() * (DA_PNN or DA_PN))
                entryPC
            }
            ZX_DEBUGGER_MODE_SP    -> { correctSP(); entrySP }
            ZX_DEBUGGER_MODE_DT    -> { flg = countData; correctDT(); entryData }
            else                   -> error("Wrong debugger mode!")
        }
        repeat(countItems) { item ->
            val str = ZxWnd.zxDebuggerString(mode, entry, flg)
            (getChildAt(item) as? Text)?.text = str
            itemAddr[item] = entry
            if(entry == selItem) {
                if(mode == ZX_DEBUGGER_MODE_PC && flags test ZX_SEL) {
                    val posMnemonic = ZxWnd.read8(ZX_PROP_JNI_RETURN_VALUE + 1)
                    wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_DEBUGGER_ASSEMBLER_TEXT.ordinal, o = str.substring(posMnemonic))
                }
                selectItem(item)
            }
            entry += ZxWnd.read8(ZX_PROP_JNI_RETURN_VALUE)
        }
    }

    private fun selectItem(idx: Int) {
/*        if(idx > (countItems / 2)) {
            entryPC = itemAddr[1]
            update(0, ZX_LIST)
        } else */
        selItem = itemAddr[idx]
        selView = getChildAt(idx) as? ItemView
        selView?.background = selector
    }

}
