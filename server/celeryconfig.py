from celery.schedules import crontab
import tasks

broker='redis://localhost:6379/0'
backend='redis://localhost:6379/1'
timezone='America/New_York'
schedule= {
            'periodic-task': {
                'task': 'tasks.periodic_task',
                #'schedule': crontab(minute=41, hour=18),
                'schedule': 10.0,
                #'options': {
                #    'expires':15.0,
                #},
            },
        }

