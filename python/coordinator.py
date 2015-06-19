# Python script for XBee communication between server on the coordinator and the Arduino on the smart door
# radhi-hersemiaji-kartowisastro 2015
# Arduino payload data consists of tag NFC UID (4 bytes hex) and relay status (1 byte hex)

from xbee import ZigBee
import time
import datetime
import serial
import MySQLdb

while True:
    try:
        # Select the correct port
        # To list the available ports, run from the terminal:
        # python -m serial.tools.list_ports
        PORT = '/dev/cu.usbserial-A7036DFF'

        # Set baud rate
        BAUD_RATE = 9600

        # Open serial port
        ser = serial.Serial(PORT, BAUD_RATE)

        # Create API object
        xbee = ZigBee(ser,escaped=True)


        def hex(bindata):
            return ''.join('%02x' % ord(byte) for byte in bindata)

        time = datetime.datetime.now()
        response = xbee.wait_read_frame()
        sa = hex(response['source_addr_long'][0:])
        rf = hex(response['rf_data'])
        uid =hex(response['rf_data'][0:4])
        statusRead = hex(response['rf_data'][4:5])
        if statusRead == 'aa':
            status = 'in'
        else:
            status = 'out'

        # print response
        print sa, rf, time, uid, status

        try:
            # Connect to the database
            db = MySQLdb.connect(host='127.0.0.1', user='root',passwd='',db='smart_lab')
            cur=db.cursor()
            cur.execute("SELECT uid FROM room_access ")
            rows = cur.fetchall()

            try:
                cur.execute("INSERT INTO room_access (date_time,room_id,uid,status) VALUES(%s,%s,%s,%s)"
                    ,(time,sa,uid,status))
                db.commit()
            except:
                db.rollback()    
                db.close
                

            for row in rows:
                uidTemp = row[0]
                if uidTemp == uid:
                    try:
                        cur.execute("UPDATE room_access SET status=%s,date_time=%s WHERE uid=%s"
                            ,(status,time,uid))
                        db.commit()
                    except:
                        db.rollback()
                else:
                    try:
                        cur.execute("INSERT INTO room_access (date_time,room_id,uid,status) VALUES(%s,%s,%s,%s)"
                            ,(time,sa,uid,status))
                        db.commit()
                    except:
                        db.rollback()
                        db.close

        except:
            print "query failed"
            db.rollback()
            db.close


    except:
        print "No XBee connected!!"

time.sleep(1)
ser.close()