<!DOCTYPE html>
<html lang="en">
<head>
    <meta http-equiv="refresh" content="1" >
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="">
    <meta name="author" content="">

    <title>Smart Laboratory Signage</title>

    <!-- Bootstrap Core CSS -->
    <link href="css/bootstrap.min.css" rel="stylesheet">

    <!-- Custom CSS -->
    <!-- <link href="css/1-col-portfolio.css" rel="stylesheet"> -->

    <!-- HTML5 Shim and Respond.js IE8 support of HTML5 elements and media queries -->
    <!-- WARNING: Respond.js doesn't work if you view the page via file:// -->
    <!--[if lt IE 9]>
        <script src="https://oss.maxcdn.com/libs/html5shiv/3.7.0/html5shiv.js"></script>
        <script src="https://oss.maxcdn.com/libs/respond.js/1.4.2/respond.min.js"></script>
    <![endif]-->
</head>
<body>
    <?php
        function connectDatabase()
        {
            // Make a MySQL Connection
            mysql_connect("localhost", "root", "") or die(mysql_error());
            mysql_select_db("smart_lab") or die(mysql_error());   
        }

        function callName($id)
        {
            connectDatabase();
            // Get all 'room_name' from the 'room_list' table
            $result = mysql_query("SELECT room_name FROM room_list WHERE id=$id")
            or die(mysql_error());       
            // keeps getting the next row until there are no more to get
            while($row = mysql_fetch_array( $result )) {
            // Print out the contents
            echo "{$row['room_name']}";
            }     
        }

        function callStatus($id)
        {
            connectDatabase();
            // Get the latest status
            $result = mysql_query("SELECT room_access.status FROM room_access INNER JOIN room_list
                WHERE room_list.room_id=room_access.room_id AND room_list.id=$id ORDER BY date_time DESC LIMIT 1") or die(mysql_error());  
            // keeps getting the next row until there are no more to get
            while($row = mysql_fetch_array( $result )) {
            // Print out the contents
            echo "{$row['status']}";
            }
        }      
    ?>

    <!-- Page Content -->
    <div class="container">

        <!-- Heading -->
        <div class="jumbotron" align="center">
            <h1>LABORATORIUM<br>SISTEM ELEKTRONIS</h1>
            <!-- <p>A digital signage framework for smart laboratory.</p> -->
            <h2 id="clockbox"></h2>
        </div>

        <!-- First Lecturer -->
        <div class="row">
            <div class="text-uppercase">

                <!-- Image -->
                <div class="col-md-2">
                    <a>
                        <img class="img-responsive" src="/signagev2/images/default.jpg" alt="LecturerA"/>
                    </a>
                </div>

                <!-- Name -->
                <div class="col-md-8">
                    <h1 class="list-group-item" align="center">
                        <?php
                            callName(1)
                        ?>
                    </h1>
                </div>

                <!-- Status -->
                <div class="col-md-2">
                    <h1 class="list-group-item" align="center">
                        <?php
                            callStatus(1)
                        ?>
                    </h1>
                </div>  

            </div>
        </div>
        <!-- /.row -->

        <hr>

        <!-- Second Lecturer -->
        <div class="row">
            <div class="text-uppercase">

                <!-- Image -->
                <div class="col-md-2">
                    <a>
                        <img class="img-responsive" src="/signagev2/images/default.jpg" alt="LecturerB"/>
                    </a>
                </div>

                <!-- Name -->
                <div class="col-md-8">
                    <h1 class="list-group-item" align="center">
                        <?php
                            callName(2)
                        ?>
                    </h1>
                </div>

                <!-- Status -->
                <div class="col-md-2">
                    <h1 class="list-group-item" align="center">
                        <?php
                            callStatus(2)
                        ?>
                    </h1>
                </div>  

            </div>
        </div>
        <!-- /.row -->

        <hr>

        <!-- Footer -->
        <footer>
            <div class="row">
                <div class="col-lg-12" align="center">
                    <p> Copyright &copy; 2015 - Radhi Hersemiaji Kartowisastro - Final Project </p>
                    <p> E-SYSTEMS LAB JTETI FT UGM 2015 </p>
                </div>
            </div>
            <!-- /.row -->
        </footer>

    </div>
    <!-- /.container -->

    <!-- jQuery -->
    <script src="js/jquery.js"></script>

    <!-- Bootstrap Core JavaScript -->
    <script src="js/bootstrap.min.js"></script>

    <!-- Date, time, clock JavaScript http://www.ricocheting.com/code/javascript/html-generator/date-time-clock -->
    <script type="text/javascript">
    tday=new Array("Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu");
    tmonth=new Array("Januari","Februari","Maret","April","Mei","Juni","Juli","Agustus","September","Oktober","November","Desember");

    function GetClock(){
    var d=new Date();
    var nday=d.getDay(),nmonth=d.getMonth(),ndate=d.getDate(),nyear=d.getYear(),nhour=d.getHours(),nmin=d.getMinutes(),nsec=d.getSeconds(),ap;

    if(nhour==0){ap=" AM";nhour=12;}
    else if(nhour<12){ap=" AM";}
    else if(nhour==12){ap=" PM";}
    else if(nhour>12){ap=" PM";nhour-=12;}

    if(nyear<1000) nyear+=1900;
    if(nmin<=9) nmin="0"+nmin;
    if(nsec<=9) nsec="0"+nsec;

    document.getElementById('clockbox').innerHTML=""+tday[nday]+", "+ndate+" "+tmonth[nmonth]+" "+nyear+" "+nhour+":"+nmin+":"+nsec+ap+"";
    }

    window.onload=function(){
    GetClock();
    setInterval(GetClock,1000);
    }
    </script>
    <!-- ./Date, time, clock JavaScript -->

</body>
</html>