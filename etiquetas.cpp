#include "etiquetas.h"
#include "ui_etiquetas.h"
#include <QtPrintSupport>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QtSql>
#include <QDebug>
#include <QPainter>
#include "code128.c"

Etiquetas::Etiquetas(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Etiquetas)
{
    ui->setupUi(this);


    /// ---------------  Inicializacion combo impresoras
    QStringList impresoras_disponibles = QPrinterInfo::availablePrinterNames();
    ui->combo_impresoras->addItems(impresoras_disponibles);
    bool flag_eva = false;
    for(int i = 0; i < impresoras_disponibles.length(); i++){
        if( impresoras_disponibles.at(i).contains("EVA") ){
            ui->combo_impresoras->setCurrentText( impresoras_disponibles.at(i) );
            flag_eva = true;
        }
    }
    if(!flag_eva){
        ui->combo_impresoras->setCurrentText(QPrinterInfo::defaultPrinterName());
    }

    /// ---------------  Inicializar vistaprevia y lista
    clave_actual = "";
    delete_custom();
    update_previa();
    ui->lista->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    ui->lista->header()->setSectionResizeMode(2,QHeaderView::Stretch);

    /// --------------- FORMATOS
    QDir dir = QDir("formatos");
    if(!dir.exists()){
        QMessageBox msg;
        msg.setText("No se encontro la carpeta de formatos!");
        msg.exec();
    }else{
        QStringList formatos = dir.entryList( QDir::NoDotAndDotDot | QDir::Files );
        ui->combo_formatos->addItems(formatos);
    }

    ui->input_descripcion->setFocus();
}

void Etiquetas::update_previa(){

    /// ---------------  ConectarDB
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    QString ip = "192.168.0.105";
    QString port = "3306";
    QString base = "sicar";
    QString username = "consultas";
    QString password = "123456";
    QFile fileip("ip_server.txt");
    if (fileip.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream inip(&fileip);
        ip = inip.readLine();
        port = inip.readLine();
        base = inip.readLine();
        username = inip.readLine();
        password = inip.readLine();
        fileip.close();
    }
    db.setHostName(ip);
    db.setPort(port.toInt());
    db.setDatabaseName(base);
    db.setUserName(username);
    db.setPassword(password);
    db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=5");

    if(!db.open()){
        QMessageBox messageBox;
        messageBox.critical(0,"Error","No se pudo conectar a la base de datos !");
        messageBox.setFixedSize(500,200);
        return;
    }

    QSqlQuery consulta;
    consulta.exec("SELECT clave,claveAlterna,descripcion,precio1,impuesto FROM articulo a LEFT JOIN articuloimpuesto ai ON a.art_id=ai.art_id LEFT JOIN impuesto i ON i.imp_id=ai.imp_id WHERE clave=\"" + clave_actual + "\" OR claveAlterna=\"" + clave_actual + "\" AND a.status!=-1");
    qDebug() << consulta.lastError().text();
    qDebug() << "Actualizando para : " + clave_actual;

    QPixmap imagen(ancho, alto);
    QPainter documento;
    documento.begin(&imagen);
    documento.fillRect( QRect(0,0,ancho,alto), QColor(255,255,255) );

    /// --------- FUENTE 1 (no encontrado)
    QFontDatabase::addApplicationFont(":/fonts/LucidaTypewriterRegular.ttf");
    QFont fuente1 = QFont("Lucida Sans Typewriter",30,QFont::Normal,false);
    fuente1.setStretch(QFont::Condensed);
    QFontMetrics metricas1 = QFontMetrics(fuente1);

    /// --------- FUENTE 2 (descripcion)
    QFontDatabase::addApplicationFont(":/fonts/LucidaTypewriterRegular.ttf");
    QFont fuente2 = QFont("Lucida Sans Typewriter",24,QFont::Normal,false);
    fuente2.setStretch(QFont::Condensed);
    QFontMetrics metricas2 = QFontMetrics(fuente2);

    /// --------- FUENTE 3 (codigo texto)
    QFont fuente3 = QFont("Consolas",20,QFont::Normal,false);
    fuente3.setStretch(QFont::Condensed);
    QFontMetrics metricas3 = QFontMetrics(fuente3);

    /// --------- FUENTE 4 (precio)
    QFontDatabase::addApplicationFont(":/fonts/ElliotSans-Bold.ttf");
    QFont fuente4 = QFont("ElliotSans-Bold",48,QFont::Normal,false);
    fuente4.setStretch(QFont::Condensed);
    QFontMetrics metricas4 = QFontMetrics(fuente4);

    /// --------- FUENTE 5 (clave alterna)
    QFontDatabase::addApplicationFont(":/fonts/AGENCYR.TTF");
    QFont fuente5 = QFont("Agency FB",42,QFont::Normal,false);
    fuente5.setStretch(QFont::Condensed);
    QFontMetrics metricas5 = QFontMetrics(fuente5);

    /// ---------- NO ENCONTRADO
    if( !consulta.next() || clave_actual == ""){
        qDebug() << "No se encontro";
        QString msg = "NO ENCONTRADO";
        documento.setFont(fuente1);
        documento.drawText( (ancho - metricas1.horizontalAdvance(msg))/2, (alto-metricas1.height())/2 + metricas1.height(), msg );
        ui->previa->setPixmap(imagen);
        return;
    }

    /// ----------- ENCONTRADO!!!
    qDebug() << "Encontrado";
    QFile file("formatos/" + ui->combo_formatos->currentText());
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        ui->previa->setPixmap(imagen);
        return;
    }
    QTextStream in(&file);
    QString line;

    while ( !in.atEnd() ) {
        line = in.readLine();
        if( line.startsWith("#") || line.trimmed() == ""){
            continue;
        }else{
            int r[4];
            leer_posicion(line, r);
            qDebug() << "Se dibujara en " << r[param_x] << ", " << r[param_y] << ", ";
            qDebug() << "Con ancho : " << r[param_ancho] << " y alto :" << param_alto;
            //////////////////////////////// CODEBAR ///////////////////////////////////
            if(line.contains("codigo_barras")){
                qDebug() << "Un codigo de barras";

                QString codigo;
                if(custom_code.isEmpty()){
                    codigo = consulta.value("clave").toString();
                }else{
                    codigo = custom_code;
                }

                if( ui->check_forzar_precio->isChecked() ){
                    codigo = ui->input_precio->text() + "/" + codigo;
                }
                QByteArray str_array = codigo.toLocal8Bit();
                const char *str = str_array.data();
                size_t barcode_length = code128_estimate_len(str);
                char *barcode_data = (char *) malloc(barcode_length);
                barcode_length = code128_encode_raw(str, barcode_data, barcode_length);

                //QMessageBox messageBox;
                //messageBox.critical(0,"Error","Debug !\nstr: "+QString(str)+"\nbarcode_lenght : "+QVariant(barcode_length).toString()+"\nbarcode data : "+QString(barcode_data)+"\n");

                int barwidth;
                if(barcode_length == 0){
                    barwidth = 1;
                }else{
                    barwidth = r[param_ancho]/barcode_length;
                }

                /* barcode_length is now the actual number of "bars". */
                qDebug() << "NumBarras : " << barcode_length << " AnchoCU : " << barwidth;

                if(barwidth > 0){
                    int bias_x = (r[param_ancho] - barwidth*barcode_length) /2;
                    for (int i = 0; i < barcode_length; i++) {
                        if (barcode_data[i])
                            documento.fillRect( QRect(r[param_x]+bias_x+i*barwidth, r[param_y],barwidth, r[param_alto] ), QColor(0,0,0) );
                    }
                }else{
                    documento.setFont(fuente1);
                    documento.drawText( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]), Qt::AlignCenter ,"NO CABE");
                }
            }
            if(line.contains("descripcion")){
                qDebug() << "La descripcion";
                QString descripcion;
                if(custom_desc.isEmpty()){
                    descripcion = consulta.value("descripcion").toString();
                }else{
                    descripcion = custom_desc;
                }
                documento.setFont(fuente2);
                documento.drawText( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]),
                                   Qt::TextWordWrap | Qt::AlignVCenter,
                                   descripcion);
            }
            if(line.contains("codigo_texto")){
                qDebug() << "El texto debajo del codigo";
                QString codigo;

                if(custom_code.isEmpty()){
                    codigo = consulta.value("clave").toString();
                }else{
                    codigo = custom_code;
                }

                if( ui->check_forzar_precio->isChecked() ){
                    codigo = ui->input_precio->text() + "/" + codigo;
                }
                documento.fillRect( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]), QColor(255,255,255) );
                documento.setFont(fuente3);
                documento.drawText( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]),
                                   Qt::AlignHCenter,
                                   codigo);
            }
            if(line.contains("clave_alterna")){
                qDebug() << "La clave alterna";
                documento.setFont(fuente5);
                documento.drawText( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]),
                                   consulta.value("claveAlterna").toString());
            }
            if(line.contains("logo1")){
                qDebug() << "Un logo1";
                QPixmap logo = QPixmap(":/logos/logo1.png");
                QRectF target(r[param_x], r[param_y], r[param_ancho], r[param_alto]);
                QRectF source(0, 0, logo.width(), logo.height());
                documento.drawPixmap(target, logo, source);
            }
            if(line.contains("logo2")){
                qDebug() << "Un logo2";
                QPixmap logo = QPixmap(":/logos/logo2.png");
                QRectF target(r[param_x], r[param_y], r[param_ancho], r[param_alto]);
                QRectF source(0, 0, logo.width(), logo.height());
                documento.drawPixmap(target, logo, source);
            }
            if(line.contains("precio")){
                qDebug() << "Un precio";
                documento.setFont(fuente4);
                if(ui->check_forzar_precio->isChecked()){
                    float precio = ui->input_precio->text().toFloat();
                    QString ps;
                    ps.setNum(precio, 'f', 2);
                    documento.drawText( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]), Qt::AlignHCenter ,"$ "+ ps);
                }else{
                    float impuesto = 1;
                    if( !consulta.value("impuesto").toString().isEmpty() ){ impuesto = impuesto + consulta.value("impuesto").toFloat()/100.0; }
                    float precio = consulta.value("precio1").toFloat() * impuesto;
                    QString ps;
                    ps.setNum(precio, 'f', 2);
                    documento.drawText( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]), Qt::AlignHCenter,"$ "+ ps);
                }

            }
            if(line.contains("rect")){
                qDebug() << "Un rectangulo";
                documento.fillRect( QRect(r[param_x],r[param_y],r[param_ancho],r[param_alto]), QColor(0,0,0) );
            }
        }
    }
    file.close();
    ui->previa->setPixmap(imagen);
}

void Etiquetas::leer_posicion(QString linea, int* posicion){
    qDebug() << "La linea es : " << linea;
    linea = linea.right( linea.length() - linea.indexOf(":") - 1 );
    qDebug() << linea;
    linea = linea.remove(linea.length()-1,1);
    qDebug() << linea;
    QStringList splitted = linea.split(",", Qt::SkipEmptyParts);
    for(int i = 0; i < 4; i++){
        qDebug() << splitted.at(i);
        *(posicion + i) = splitted.at(i).toInt();
    }
}

Etiquetas::~Etiquetas(){
    delete ui;
}

void Etiquetas::delete_custom(){
    custom_code = "";
    custom_desc = "";
}

void Etiquetas::on_input_descripcion_returnPressed()
{
    ui->lista->setEnabled(true);
    ui->lista->clear();
    QString text = ui->input_descripcion->text();
    QStringList palabras = text.split(" ",Qt::SkipEmptyParts);

    if( palabras.length() > 0 ){
        qDebug() << "Consultando";

        /// ---------------  ConectarDB
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
        QString ip = "192.168.0.105";
        QString port = "3306";
        QString base = "sicar";
        QString username = "consultas";
        QString password = "123456";
        QFile file("ip_server.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
            QTextStream in(&file);
            ip = in.readLine();
            port = in.readLine();
            base = in.readLine();
            username = in.readLine();
            password = in.readLine();
            file.close();
        }
        db.setHostName(ip);
        db.setPort(port.toInt());
        db.setDatabaseName(base);
        db.setUserName(username);
        db.setPassword(password);
        db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=5");

        if(!db.open()){
            QMessageBox messageBox;
            messageBox.critical(0,"Error","No se pudo conectar a la base de datos !");
            messageBox.setFixedSize(500,200);
            return;
        }

        QString select = "SELECT clave,claveAlterna,descripcion,precio1,impuesto FROM articulo a LEFT JOIN articuloimpuesto ai ON a.art_id=ai.art_id LEFT JOIN impuesto i ON i.imp_id=ai.imp_id WHERE ";
        select += "(descripcion LIKE \"%" + palabras.at(0) + "%\" OR clave LIKE \"%" + palabras.at(0) + "%\" OR claveAlterna LIKE \"%" + palabras.at(0) + "%\") ";
        for( int i = 1; i < palabras.length(); i++){
            select += "AND (descripcion LIKE \"%" + palabras.at(i) + "%\" OR clave LIKE \"%" + palabras.at(i) + "%\" OR claveAlterna LIKE \"%" + palabras.at(i) + "%\") ";
        }
        select += "AND a.status=1 ORDER BY a.art_id DESC";
        QSqlQuery consulta;
        consulta.exec(select);
        while(consulta.next()){
            QStringList item;
            item << consulta.value("clave").toString();
            item << consulta.value("claveAlterna").toString();
            item << consulta.value("descripcion").toString();

            float impuesto = 1;
            if( !consulta.value("impuesto").toString().isEmpty() ){ impuesto = impuesto + consulta.value("impuesto").toFloat()/100.0; }
            float precio1_float = consulta.value("precio1").toFloat() * impuesto;
            QString precio1;
            precio1 = precio1.setNum(precio1_float,'f',2);
            item << precio1;

            ui->lista->addTopLevelItem(new QTreeWidgetItem( item ));
        }
        qDebug() << "Consulta finalizada";
    }
}

void Etiquetas::on_input_clave_returnPressed()
{
    clave_actual = ui->input_clave->text().trimmed();
    delete_custom();
    update_previa();
    ui->input_clave->selectAll();
}


void Etiquetas::on_lista_itemClicked(QTreeWidgetItem *item, int column)
{
    clave_actual = item->text(0);
    delete_custom();
    update_previa();
}


void Etiquetas::on_boton_imprimir_clicked()
{
    QPrinter p = QPrinter( QPrinterInfo::printerInfo(ui->combo_impresoras->currentText()), QPrinter::HighResolution);
    QList<int> sr = p.supportedResolutions();
    for(int i = 0; i < sr.length() ; i++){
        qDebug() << sr.at(i);
    }
    QPainter documento;
    QPixmap imagen = ui->previa->pixmap(Qt::ReturnByValue);

    ui->spin_etiquetas->setEnabled(false);

    for(int i = 0; i < ui->spin_etiquetas->value(); i++){
        qDebug() << "Ancho : " << imagen.width();
        qDebug() << "Alto : " << imagen.height();
        documento.begin(&p);
        documento.drawPixmap(7,5,ancho,alto,imagen);
        documento.end();
    }

    ui->spin_etiquetas->setEnabled(true);
    ui->spin_etiquetas->setValue(1);
}

void Etiquetas::on_input_precio_textChanged(const QString &arg1)
{
    update_previa();
}


void Etiquetas::on_check_forzar_precio_stateChanged(int arg1)
{
    if( ui->check_forzar_precio->isChecked() ){
        ui->input_precio->setEnabled(true);
    }else{
        ui->input_precio->setEnabled(false);
    }
    update_previa();
}


void Etiquetas::on_combo_formatos_currentTextChanged(const QString &arg1)
{
    update_previa();
}


void Etiquetas::on_lista_itemChanged(QTreeWidgetItem *item, int column)
{
    clave_actual = item->text(0);
    delete_custom();
    update_previa();
}


void Etiquetas::on_lista_itemEntered(QTreeWidgetItem *item, int column)
{
    clave_actual = item->text(0);
    delete_custom();
    update_previa();
}

void Etiquetas::on_editar_nombre_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Editar Nombre ... ", "Escriba la descripcion", QLineEdit::Normal, "", &ok);
    if ( ok ){
        custom_desc = text;
        update_previa( );
    }
}

void Etiquetas::on_editar_codigo_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Editar Codigo ... ", "Escriba el codigo nuevo", QLineEdit::Normal, "", &ok);
    if ( ok ){
        custom_code = text;
        update_previa( );
    }
}

