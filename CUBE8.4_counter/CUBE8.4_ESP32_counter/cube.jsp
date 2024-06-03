<%@ page language="java" contentType="text/html; charset=UTF-8"
    pageEncoding="UTF-8"%>
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>CUBE</title>
</head>
<body>
  <h1>Contagem:</h1>
  <%
    //Recebe o valor da variável enviado pelo ESP32
    String machine = request.getParameter("machine");
    String count   = request.getParameter("count");
    System.out.print("\nMACHINE ["+machine+"] CICLOS ["+count+"] \n");
  %>
</body>
</html>