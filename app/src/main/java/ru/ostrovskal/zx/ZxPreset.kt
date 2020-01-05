package ru.ostrovskal.zx

import ru.ostrovskal.sshstd.sql.Table
import ru.ostrovskal.sshstd.utils.toIntArray
import ru.ostrovskal.zx.ZxCommon.ZX_PROP_JOY_KEYS
import ru.ostrovskal.zx.ZxCommon.ZX_PROP_JOY_TYPE

object ZxPreset: Table() {
    @JvmField val id  = integer("_id").primaryKey(1)
    @JvmField val prgName= text("name").notNull
    @JvmField val joyType= integer("jtype").notNull.default(0)
    @JvmField val joyKeys= text("jkeys").notNull.default("47,48,49,50,51,0,0,0")

    // загружаем все пресеты
    fun list() = ZxPreset.listOf(prgName, prgName, true)

    // загружаем пресет для данной проги
    fun load(name: String) {
        select(joyType, joyKeys) { where { prgName eq name } }.execute {
            ZxWnd.props[ZX_PROP_JOY_TYPE] = integer(0).toByte()
            text(1).toIntArray(8, 0, 10, false, ',').forEachIndexed { index, i ->
                ZxWnd.props[ZX_PROP_JOY_KEYS + index] = i.toByte()
            }

        }
    }

    // сохраняем/обновляем пресет для данной проги
    fun save() {
        val name = ZxWnd.zxProgramName("")
        insertOrUpdate( { prgName eq name } ) {
            var skeys = ""
            repeat(8) { idx ->
                if(skeys.isNotEmpty()) skeys += ","
                skeys += ZxWnd.props[ZX_PROP_JOY_KEYS + idx].toString()
            }
            values[prgName] = name
            values[joyType] = ZxWnd.props[ZX_PROP_JOY_TYPE]
            values[joyKeys] = skeys
        }
    }
}
