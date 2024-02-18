#ifndef ETIQUETAS_H
#define ETIQUETAS_H

#include <QMainWindow>
#include <QtSql>
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class Etiquetas; }
QT_END_NAMESPACE

class Etiquetas : public QMainWindow
{
    Q_OBJECT

public:
    Etiquetas(QWidget *parent = nullptr);
    ~Etiquetas();

private slots:

    void on_input_descripcion_returnPressed();

    void on_input_clave_returnPressed();

    void on_lista_itemClicked(QTreeWidgetItem *item, int column);

    void on_boton_imprimir_clicked();

    void on_input_precio_textChanged(const QString &arg1);

    void on_check_forzar_precio_stateChanged(int arg1);

    void on_combo_formatos_currentTextChanged(const QString &arg1);

    void on_lista_itemChanged(QTreeWidgetItem *item, int column);

    void on_lista_itemEntered(QTreeWidgetItem *item, int column);

    void delete_custom();

    void on_editar_nombre_clicked();

    void on_editar_codigo_clicked();

private:
    Ui::Etiquetas *ui;

    void update_previa();

    QString clave_actual = "";
    QString custom_desc = "";
    QString custom_code = "";

    void leer_posicion(QString linea, int* posicion);

    int ancho = 400;
    int alto = 200;

    int param_x = 0;
    int param_y = 1;
    int param_ancho = 2;
    int param_alto = 3;
};
#endif // ETIQUETAS_H
