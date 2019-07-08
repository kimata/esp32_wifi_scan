import React from 'react';
import './bootstrap.min.css';
import './bootstrap-reboot.min.css';
import './bootstrap-grid.min.css';

import Jumbotron from 'react-bootstrap/Jumbotron'
import Spinner from 'react-bootstrap/Spinner'
import Table from 'react-bootstrap/Table'
import ProgressBar from 'react-bootstrap/ProgressBar'
import Alert from 'react-bootstrap/Alert'

class App extends React.Component {
  render() {
    return (
      <div className="App container">
        <Jumbotron>
          <h1>WiFi AP list</h1>
          <p>Displaying a list of WIFI access points detected by ESP32.</p>
        </Jumbotron>
        <WiFiApInfo />
      </div>    
    );
  }
}

class WiFiApInfo extends React.Component {
  constructor(props) {
    super(props);
    this.state={
      loading: true
    };
  }

  componentDidMount() {
    return fetch('/api/')
      .then((res) => res.json())
      .then((resJson) => {
        this.setState({
          loading: false,
          data: resJson,
          error: null,
        });
      })
      .catch((error) =>{
        this.setState({
          loading: false,
          error: error,
        });
      });
  }

  loading() {
    return (<Spinner animation="border" variant="primary" />)
  }

  list() {
    return (
        <Table striped bordered hover className="table">
          <thead>
            <tr>
              <th width="10%">CH</th>
              <th width="60%">SSID</th>
              <th>RSSI</th>
          </tr>
        </thead>
          <tbody>
            {
              this.state.data.map(function(apInfo){
                return (
                  <tr>
                    <td className="align-middle">{apInfo.ch}</td>
                    <td className="align-middle text-left">{apInfo.ssid}</td>
                    <td className="align-middle">
                      <ProgressBar variant="success" label={apInfo.rssi}
                                   now={140 + apInfo.rssi} 
                                   style={{height: 2 + 'em',
                                           fontSize: 1 + 'em' }} />
                    </td>
                  </tr>
                );
              })
            }                 
        </tbody>
      </Table>
    )
  }
  render() {
    if (this.state.loading) {
      return this.loading();
    } else if (this.state.error != null) {
      return (<Alert variant="danger">情報の取得に失敗しました．</Alert>);
    } else {
      return this.list();
    }
  }
}



export default App;

// Local Variables:
// mode: rjsx
// coding: utf-8-unix
// End:
