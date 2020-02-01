package ru.ostrovskal.zx

import ru.ostrovskal.sshstd.sql.Table

object ZxPoke: Table() {
    @JvmField val id = integer("_id").primaryKey(1)
    @JvmField val prgName = text("name").notNull
    @JvmField val joyType = integer("jtype").notNull.default(0)
    @JvmField val joyKeys = text("jkeys").notNull.default("47,48,49,50,51,0,0,0")
}