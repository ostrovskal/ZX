package ru.ostrovskal.zx

import android.content.Context
import ru.ostrovskal.sshstd.sql.Table
import ru.ostrovskal.zx.ZxCommon.ZX_PROP_JOY_KEYS
import ru.ostrovskal.zx.ZxCommon.ZX_PROP_JOY_TYPE

object ZxPreset: Table() {
    @JvmField val id  = integer("_id").primaryKey(1)
    @JvmField val prgName= text("name").notNull
    @JvmField val joyType= integer("jtype").notNull
    @JvmField val joyKeys= text("jkeys").notNull

    private fun setJoy(type: Int, keys: String) {
        ZxWnd.props[ZX_PROP_JOY_TYPE] = type.toByte()
        keys.split(",").forEachIndexed { index, i ->
            ZxWnd.props[ZX_PROP_JOY_KEYS + index] = i.toByte()
        }
    }

    // добавить джойстики по умолчанию для "моих" игр
    fun setDefaults(context: Context) {
        // name.type-l,r,u,d,x,y,a,b
        context.assets.open("joys.txt").reader().forEachLine { str ->
            insertOrUpdate( { prgName eq name } ) {
                values[prgName] = str.substringBefore('.')
                values[joyType] = str.substringAfter('.').substringBefore('-').toInt()
                values[joyKeys] = str.substringAfter('-')
            }
        }
    }

    // загружаем все пресеты
    fun list() = ZxPreset.listOf(prgName, prgName, true)

    // загружаем пресет
    fun load(name: String) {
        var namePrg = name
        if(name.isBlank()) namePrg = ZxWnd.zxProgramName("")
        // сначала ставим джойстик по умолчанию - а потом загружаем(если есть)
        setJoy(0, "48,49,50,51,52,38,22,10")
        select(joyType, joyKeys) { where { prgName eq namePrg } }.execute {
            setJoy(integer(0), text(1))
        }
    }

    // сохраняем/обновляем пресет
    fun save() {
        val namePrg = ZxWnd.zxProgramName("")
        insertOrUpdate( { prgName eq namePrg } ) {
            var skeys = ""
            repeat(8) { idx ->
                if(skeys.isNotEmpty()) skeys += ","
                skeys += ZxWnd.props[ZX_PROP_JOY_KEYS + idx].toString()
            }
            values[prgName] = namePrg
            values[joyType] = ZxWnd.props[ZX_PROP_JOY_TYPE]
            values[joyKeys] = skeys
        }
    }
}
