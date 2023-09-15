import serial.tools.list_ports
import serial
import time
import logging
from telegram import Bot, ReplyKeyboardMarkup
from telegram.ext import CommandHandler, Updater

"""_log_format = (
    '%(asctime)s, %(levelname)s,'
    ' %(message)s - (%(filename)s).%(funcName)s(%(lineno)d)'
)"""

_log_format = ('%(message)s')

def get_stream_handler() -> logging.Handler:
    """ Функция устанавливает stream handler
        для логгирования.
    """
    stream_handler = logging.StreamHandler()
    stream_handler.setLevel(logging.INFO)
    stream_handler.setFormatter(logging.Formatter(_log_format))
    return stream_handler


def get_logger(name: str) -> logging.Logger:
    """ Функция формирует логгер.
    """
    logger = logging.getLogger(name)
    logger.setLevel(logging.INFO)
    logger.addHandler(get_stream_handler())
    return logger


logger = get_logger(__name__)

def get_data() -> str:
    """ Функция получает данные
        с COM-порта.
    """
    ports = serial.tools.list_ports.comports(include_links=False)
    for port in ports:
        try:
            logger.info(port)
            kwargs = {
                'port': port.device,
                'baudrate': 9600,
                'timeout': 5
            }
            ser = serial.Serial(**kwargs)
        except Exception as error:
            logger.error('Не удалось соединиться с портом.')
        else:
            data = ser.readline().decode('utf-8')
            ser.close()
            return data

def parse_data(data: str) -> dict:
    """ Функция парсит данные,
        полученные с COM-порта.
    """
    dic = {}
    strings = data.split(';')
    strings.pop()
    logger.info(strings)
    for string in strings:
        bufs = string.split(':')
        dic[bufs[0]] = bufs[1]
    return dic

def prepare_message(data: dict) -> str:
    """ Функция подготавливает
        сообщение для отправки.
    """
    error = 'Не удалось считать данные'
    if not 'temp_in_kitchen' in data:
        data['temp_in_kitchen'] = error
    if not 'temp_in_basement' in data:
        data['temp_in_basement'] = error
    if not 'wet_in_basement' in data:
        data['wet_in_basement'] = error
    if not 'temp_in_attic' in data:
        data['temp_in_attic'] = error
    if not 'gas' in data:
        data['gas'] = error
    if not 'secure_mode' in data:
        data['secure_mode'] = error
    if not 'timeofday' in data:
        data['timeofday'] = error
    message = ''
    message += f"Основные параметры системы\n"
    message += f"Охранная сигнализация: {data['secure_mode']}\n"
    message += f"Вечернее освещение: {data['timeofday']}\n"
    message += f"Температура в подвале: {data['temp_in_basement']}\n"
    message += f"Температура на чердаке: {data['temp_in_attic']}\n"
    message += f"Температура на кухне: {data['temp_in_kitchen']}\n"
    message += f"Влажность в подвале: {data['wet_in_basement']}\n"
    message += f"Наличие газа на кухне: {data['gas']}\n"
    return message

updater = Updater(token='5689319658:AAHAyqYOz4yC_HE9Js5LE3GT8g7w4aVXkdA')
def wake_up(update, context):
    chat = update.effective_chat
    buttons = ReplyKeyboardMarkup([
        ['/get_all_data'],
        ['/set_secure_mode']
    ])
    context.bot.send_message(
        chat_id=chat.id,
        text='Добро пожаловать!',
        reply_markup=buttons
    )
def get_all_data(update, context):
    data = get_data()
    parse = parse_data(data)
    message = prepare_message(parse)
    chat = update.effective_chat
    context.bot.send_message(
        chat_id=chat.id,
        text=message
    )
def set_secure_mode(update, context):
    chat = update.effective_chat
    message = 'Установлен охранный режим'
    context.bot.send_message(
        chat_id=chat.id,
        text=message
    )

updater.dispatcher.add_handler(CommandHandler('start', wake_up))
updater.dispatcher.add_handler(CommandHandler('get_all_data', get_all_data))
updater.dispatcher.add_handler(CommandHandler('set_secure_mode', set_secure_mode))
updater.start_polling()
updater.idle()